//
// SimpleVolume
//
#include <sys/stat.h>
#include "SimpleVolume.h"
#include "nvrRender.h"
#include "utilEndian.h"

using namespace NVR;
using namespace CES;

bool SimpleVolume::AllocData(const NVR::Dim3& dims)
{
  size_t dlen = dims.Size();
  if ( dlen < 1 ) {
    m_dims = Dim3(0,0,0);
    return true;
  }

  switch ( m_format ) {
  case UNSIGNED_SHORT:
    dlen *= sizeof(unsigned short); break;
  case UNSIGNED_INT:
    dlen *= sizeof(unsigned int); break;
  case NVR::FLOAT:
    dlen *= sizeof(float); break;
  }

  m_data = ReAllocate(m_data, dlen);
  if ( ! m_data ) {
    perror("Error in NVR::SimpleVolume::AllocData: ");
    m_dims = Dim3(0,0,0);
    return false;
  }
  m_dims = dims;
  return true;
}

bool SimpleVolume::LoadVoxVol(const char* path)
{
  // open the file
  if ( ! path ) return false;
  FILE* fp = fopen(path, "rb");
  if ( ! fp ) return false;

  // check headers
  char buff[256];
  register bool littleEndian = true;
  int Lcount = 0, voxSize = 0;
  Dim3 dims;
  Vec3<float> position(0,0,0), scale(1,1,1);
  bool hasPosition(false), hasScale(false);
  size_t bit_pos = 0;
  size_t bit_len = 8;
  while ( fgets(buff, 255, fp) ) {
    if ( ! strncmp(buff, "##\f", 3) )
      Lcount ++;
    if ( Lcount == 2 )
      break;

    if ( ! strncmp(buff, "VolumeSize", 10) )
      sscanf(&buff[11], "%d %d %d",
	     &dims.size[0], &dims.size[1], &dims.size[2]);
    else if ( ! strncmp(buff, "VoxelSize", 9) )
      sscanf(&buff[10], "%d", &voxSize);
    else if ( ! strncmp(buff, "VolumePosition", 14) ) {
      sscanf(&buff[15], "%f %f %f",
	     &position.m_v[0], &position.m_v[1], &position.m_v[2]);
      hasPosition = true;
    }
    else if ( ! strncmp(buff, "VolumeScale", 11) ) {
      sscanf(&buff[12], "%f %f %f",
	     &scale.m_v[0], &scale.m_v[1], &scale.m_v[2]);
      hasScale = true;
    }
    else if ( ! strncmp(buff, "Endian", 6) ) {
      char syms[32];
      if ( sscanf(&buff[7], "%s", syms) > 0 ) {
	if ( syms[0] == 'L' || syms[0] == 'l' )
	  littleEndian = true;
	else
	  littleEndian = false;
      }
    }
    else if ( ! strncmp(buff, "Field", 5) ) {
      char* pp = strstr(buff, "Position");
      if ( ! pp ) continue;
      sscanf(pp+8, "%d", &bit_pos);

      pp = strstr(buff, "Size");
      if ( ! pp ) continue;
      sscanf(pp+4, "%d", &bit_len);
    }
  } // end of while

  if ( dims.Size() < 1 ) {
    fclose(fp);
    return false;
  }
  switch ( voxSize ) {
  case 8: case 16:
    break;
  default:
    fclose(fp);
    return false;
  }
  short bit_mask = (0xffff >> (16-bit_len));

  // allocation
  m_format = UNSIGNED_BYTE;
  if ( ! AllocData(dims) ) {
    fclose(fp);
    return false;
  }

  // bbox
  m_bbox[0] = Vec3<float>(0,0,0);
  m_bbox[1] = Vec3<float>(dims[0]-1, dims[1]-1, dims[2]-1);
  if ( hasScale ) {
    m_bbox[1][0] = m_bbox[1][0] * scale[0];
    m_bbox[1][1] = m_bbox[1][1] * scale[1];
    m_bbox[1][2] = m_bbox[1][2] * scale[2];
  }
  if ( hasPosition ) {
    m_bbox[0] = m_bbox[0] + position;
    m_bbox[1] = m_bbox[1] + position;
  }

  // conversion
  register int i, j, k;
  size_t wkSize = dims.size[0] * dims.size[1] * (voxSize/8);
  unsigned char* wkd = new unsigned char[wkSize];
  short* wks = (short*)wkd;
  unsigned char* volp = (unsigned char*)m_data;
  for ( k = 0; k < dims.size[2]; k++ ) {
    if ( ! fread(wkd, sizeof(unsigned char), wkSize, fp) ) {
      fclose(fp);
      return false;
    }
    for ( j = 0; j < dims.size[1]; j++ )
      for ( i = 0; i < dims.size[0]; i++ ) {
	if ( voxSize == 16 ) {
#ifndef i386
	  short xx = littleEndian ? BSWAP_X_16(wks[dims.size[0]*j + i])
	    : wks[dims.size[0]*j + i];
#else
	  short xx = littleEndian ? wks[dims.size[0]*j + i]
	    : BSWAP_X_16(wks[dims.size[0]*j + i]);
#endif
          short ax = (xx >> bit_pos) & bit_mask;
	  volp[dims.size[0] * dims.size[1] * k + dims.size[0]*j + i]
	    = (unsigned char)(ax * 255 / bit_mask);
	} else {
	  volp[dims.size[0] * dims.size[1] * k + dims.size[0]*j + i]
	    = wkd[dims.size[0]*j + i];
	}
      }
  }

  fclose(fp);
  delete [] wkd;
  return true;
}

bool SimpleVolume::LoadAvsVol(const char* path)
{
  // open the file
  if ( ! path ) return false;
  FILE* fp = fopen(path, "rb");
  if ( ! fp ) {
    return false;
  }

  unsigned char xdim[3];
  if ( fread(xdim, 1, 3, fp) < 3 ) {
    fclose(fp);
    return false;
  }
  Dim3 dims(xdim[0], xdim[1], xdim[2]);
  if ( dims.Size() < 1 ) {
    fclose(fp);
    return false;
  }
  m_bbox[0] = Vec3<float>(0,0,0);
  m_bbox[1] = Vec3<float>(dims[0]-1, dims[1]-1, dims[2]-1);

  m_format = UNSIGNED_BYTE;
  if ( ! AllocData(dims) ) {
    fclose(fp);
    return false;
  }

  unsigned char *datap = (unsigned char*)m_data;
  if ( fread(datap, 1, dims.Size(), fp) < dims.Size() ) {
    fclose(fp);
    return true;
  }

  fclose(fp);
  return true;
}

bool SimpleVolume::LoadFdvVol(const char* path, const size_t stp)
{
  if ( ! path ) return false;

  // open the file
  FILE* fp = fopen(path, "rb");
  if ( ! fp ) {return false;}

  // get file status for file-length
  struct stat st_buf;
  if ( fstat(fileno(fp), &st_buf) < 0 ) {fclose(fp); return false;}

  // endian check
  bool doBx = false;
  CES::EMatchType mtf = CES::MatchEndian(fp, 12);
  if ( mtf == CES::UnKnown ) return false;
  if ( mtf == CES::UnMatch ) doBx = true;

  // read dims
  int xdim[5];
  if ( fread(xdim, sizeof(int), 5, fp) < 5 ) {fclose(fp); return false;}
  if ( doBx ) BSWAPVEC(xdim, 5);
  if ( xdim[1] < 1 || xdim[2] < 1 || xdim[3] < 1 ) {fclose(fp); return false;}

  // allocate
  NVR::Dim3 dims(xdim[1], xdim[2], xdim[3]);
  if ( ! AllocData(dims) ) {fclose(fp); return false;}

  // skip to stp
  size_t sz = dims.Size();
  size_t sz4 = (sz%4==0 ? sz : sz+4-sz%4); // data size with padding
  size_t stpLen = (4 + 8 + 2)*4 + sz4;
  sz = (st_buf.st_size - 5*4) / stpLen; // #of steps
  if ( stp >= sz ) {fclose(fp); return false;}
  int i = 0;
  while ( i < stp ) {
    if ( fseek(fp, stpLen, SEEK_CUR) < 0 ) {fclose(fp); return false;}
    i++;
  }

  // skip time record
  if ( fread(xdim, sizeof(int), 4, fp) < 4 ) {fclose(fp); return false;}

  // read range (= bbox)
  if ( fread(&sz, sizeof(int), 1, fp) < 1 ) {fclose(fp); return false;}
  if ( doBx ) BSWAP32(sz);
  if ( sz != 24 ) {fclose(fp); return false;}
  if ( fread(m_bbox[0].m_v, sizeof(float), 3, fp) < 3 ) {
    fclose(fp); return false;}
  if ( doBx ) BSWAPVEC(m_bbox[0].m_v, 3);
  if ( fread(m_bbox[1].m_v, sizeof(float), 3, fp) < 3 ) {
    fclose(fp); return false;}
  if ( doBx ) BSWAPVEC(m_bbox[1].m_v, 3);
  if ( fread(&sz, sizeof(int), 1, fp) < 1 ) {fclose(fp); return false;}

  // read datas
  if ( fread(&sz, sizeof(int), 1, fp) < 1 ) {fclose(fp); return false;}
  if ( doBx ) BSWAP32(sz);
  if ( sz != sz4 ) {fclose(fp); return false;}
  if ( fread(m_data, 1, dims.Size(), fp) < dims.Size() ) {
    fclose(fp); return false;}

  fclose(fp);
  return true;
}


bool SimpleVolume::EnPower2()
{
  if ( m_dims.Size() < 1 ) return false;
  if ( ! m_data ) return false;

  if ( IsPow2(m_dims.size[0]) &&
       IsPow2(m_dims.size[1]) &&
       IsPow2(m_dims.size[2]) ) return true;

  Dim3 dims;
  for ( dims.size[0] = 4096; dims.size[0] > 1; dims.size[0] /= 2 )
    if ( dims.size[0] < m_dims.size[0] ) break;
  dims.size[0] *= 2;
  for ( dims.size[1] = 4096; dims.size[1] > 1; dims.size[1] /= 2 )
    if ( dims.size[1] < m_dims.size[1] ) break;
  dims.size[1] *= 2;
  for ( dims.size[2] = 4096; dims.size[2] > 1; dims.size[2] /= 2 )
    if ( dims.size[2] < m_dims.size[2] ) break;
  dims.size[2] *= 2;

  size_t dlen = dims.Size();
  if ( dlen < 1 ) return false;
  switch ( m_format ) {
  case UNSIGNED_SHORT:
    dlen *= sizeof(unsigned short); break;
  case UNSIGNED_INT:
    dlen *= sizeof(unsigned int); break;
  case NVR::FLOAT:
    dlen *= sizeof(float); break;
  }

  void* newData = Allocate(dlen);
  if ( ! newData ) return false;
  switch ( m_format ) {
  case UNSIGNED_BYTE:
    if ( ! copyData(dims, (unsigned char*)newData) ) {
      DeAllocate(newData); return false;
    } break;
  case UNSIGNED_SHORT:
    if ( ! copyData(dims, (unsigned short*)newData) ) {
      DeAllocate(newData); return false;
    } break;
  case UNSIGNED_INT:
    if ( ! copyData(dims, (unsigned int*)newData) ) {
      DeAllocate(newData); return false;
    } break;
  case NVR::FLOAT:
    if ( ! copyData(dims, (float*)newData) ) {
      DeAllocate(newData); return false;
    } break;
  }

  DeAllocate(m_data); m_data = newData;

  Vec3<float> bbSize = m_bbox[1] - m_bbox[0];
  bbSize[0] *= ((float)dims[0] / m_dims[0] - 1.0f);
  bbSize[1] *= ((float)dims[1] / m_dims[1] - 1.0f);
  bbSize[2] *= ((float)dims[2] / m_dims[2] - 1.0f);
  m_bbox[1] = m_bbox[1] + bbSize;

  m_dims = dims;
  return true;
}

template<class T>
bool SimpleVolume::copyData(const NVR::Dim3 dims, T* newData)
{
  if ( dims.Size() < 1 || m_dims.Size() < 1 ) return false;
  if ( ! newData || ! m_data ) return false;

  size_t dlen = dims.Size() * sizeof(T);
  memset(newData, 0, dlen);

  register size_t iy, iz;
  T* orgData = (T*)m_data;
  for ( iz = 0; iz < m_dims.size[2]; iz++ )
    for ( iy = 0; iy < m_dims.size[1]; iy++ )
      memcpy(&newData[iz*dims.size[0]*dims.size[1] + iy*dims.size[0]],
             &orgData[iz*m_dims.size[0]*m_dims.size[1]
		     + iy*m_dims.size[0]], m_dims.size[0]*sizeof(T));
  return true;
}
