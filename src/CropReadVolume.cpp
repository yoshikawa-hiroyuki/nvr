//
// CropReadVolume
//
#include <sys/stat.h>
#include "CropReadVolume.h"

using namespace NVR;
using namespace CES;


bool CropReadVolume::CropLoadVoxVol(const char* path,
				    const Dim3& crStart, const Dim3& xcrDims)
{
  // open the file
  if ( ! path ) return false;
  FILE* fp = fopen(path, "rb");
  if ( ! fp )
    return false;
  
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

  // dimension check
  Dim3 crDims(xcrDims);
  if ( xcrDims.Size() < 1 )
    crDims = Dim3(dims[0]-crStart[0], dims[1]-crStart[1], dims[2]-crStart[2]);
  if ( crDims.Size() < 1 ) {
    fclose(fp);
    return false;
  }

  Dim3 crEnd(crStart[0]+crDims[0], crStart[1]+crDims[1], crStart[2]+crDims[2]);
  if ( crEnd[0] > dims[0] || crEnd[1] > dims[1] || crEnd[2] > dims[2] ) {
    fclose(fp);
    return false;
  }

  // allocation
  m_format = UNSIGNED_BYTE;
  if ( ! AllocData(crDims) ) {
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
  m_orgBbox[0] = m_bbox[0];
  m_orgBbox[1] = m_bbox[1];
  Vec3<float> crLen = m_bbox[1] - m_bbox[0];
  Vec3<float> crsLen((float)crStart[0]/dims[0] * crLen[0],
		     (float)crStart[1]/dims[1] * crLen[1],
		     (float)crStart[2]/dims[2] * crLen[2]);
  Vec3<float> crdLen((float)crDims[0]/dims[0] * crLen[0],
		     (float)crDims[1]/dims[1] * crLen[1],
		     (float)crDims[2]/dims[2] * crLen[2]);
  m_bbox[0] = m_bbox[0] + crsLen;
  m_bbox[1] = m_bbox[0] + crdLen;

  // conversion
  register int i, j, k;
  size_t wkSize = dims[0] * (voxSize/8);
  unsigned char* wkd = new unsigned char[wkSize];
  short* wks = (short*)wkd;
  unsigned char* volp = (unsigned char*)m_data;

  // skip crStart[2] x wkSize x dims[1]
  if ( fseek(fp, wkSize*dims[1]*crStart[2], SEEK_CUR) < 0 ) {
    fclose(fp);
    return false;
  }

  // reading
  for ( k = 0; k < crDims[2]; k++ ) {
    // skip wkSize x crStart[1]
    if ( fseek(fp, wkSize*crStart[1], SEEK_CUR) < 0 ) {
      fclose(fp);
      return false;
    }

    for ( j = 0; j < crDims[1]; j++ ) {
      // read into wkd
      if ( ! fread(wkd, sizeof(unsigned char), wkSize, fp) ) {
	fclose(fp);
	return false;
      }

      // copy to volp
      for ( i = 0; i < crDims[0]; i++ ) {
        if ( voxSize == 16 ) {
#ifndef i386
          short xx = littleEndian ? BSWAP_X_16(wks[crStart[0] + i])
            : wks[crStart[0] + i];
#else
          short xx = littleEndian ? wks[crStart[0] + i]
            : BSWAP_X_16(wks[crStart[0] + i]);
#endif
          short ax = (xx >> bit_pos) & bit_mask;
          volp[crDims[0] * crDims[1] * k + crDims[0]*j + i]
            = (unsigned char)(ax * 255 / bit_mask);
        } else {
          volp[crDims[0] * crDims[1] * k + crDims[0]*j + i]
            = wkd[crStart[0] + i];
        }
      } // end of for(i)

    } // end of for(j)

    // skip wkSize x (dims[1] - crEnd[1])
    if ( fseek(fp, wkSize*(dims[1]-crEnd[1]), SEEK_CUR) < 0 ) {
      fclose(fp);
      return false;
    }

  } // end of for(k)

  fclose(fp);
  delete [] wkd;

  m_offset = crStart;
  m_orgDims = dims;
  return true;
}

//STATIC
Dim3 CropReadVolume::GetWholeSizeVox(const char* path)
{
  if ( ! path ) return Dim3(0,0,0);
  FILE* fp = fopen(path, "rb");
  if ( ! fp ) return Dim3(0,0,0);

  // check headers
  char buff[256];
  int Lcount = 0;
  Dim3 dims;
  while ( fgets(buff, 255, fp) ) {
    if ( ! strncmp(buff, "##\f", 3) )
      Lcount ++;
    if ( Lcount == 2 )
      break;

    if ( ! strncmp(buff, "VolumeSize", 10) )
      sscanf(&buff[11], "%d %d %d",
             &dims.size[0], &dims.size[1], &dims.size[2]);
  } // end of while

  fclose(fp);
  return dims;
}


bool CropReadVolume::CropLoadAvsVol(const char* path,
				    const Dim3& crStart, const Dim3& xcrDims)
{
  // open the file
  if ( ! path ) return false;
  FILE* fp = fopen(path, "rb");
  if ( ! fp )
    return false;

  // dimension check
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

  Dim3 crDims(xcrDims);
  if ( xcrDims.Size() < 1 )
    crDims = Dim3(dims[0]-crStart[0], dims[1]-crStart[1], dims[2]-crStart[2]);
  if ( crDims.Size() < 1 ) {
    fclose(fp);
    return false;
  }

  Dim3 crEnd(crStart[0]+crDims[0], crStart[1]+crDims[1], crStart[2]+crDims[2]);
  if ( crEnd[0] > dims[0] || crEnd[1] > dims[1] || crEnd[2] > dims[2] ) {
    fclose(fp);
    return false;
  }

  // allocation
  m_format = UNSIGNED_BYTE;
  if ( ! AllocData(crDims) ) {
    fclose(fp);
    return false;
  }

  // bbox
  m_bbox[0] = Vec3<float>(0,0,0);
  m_bbox[1] = Vec3<float>(dims[0]-1, dims[1]-1, dims[2]-1);
  m_orgBbox[0] = m_bbox[0];
  m_orgBbox[1] = m_bbox[1];
  Vec3<float> crLen = m_bbox[1] - m_bbox[0];
  Vec3<float> crsLen((float)crStart[0]/dims[0] * crLen[0],
		     (float)crStart[1]/dims[1] * crLen[1],
		     (float)crStart[2]/dims[2] * crLen[2]);
  Vec3<float> crdLen((float)crDims[0]/dims[0] * crLen[0],
		     (float)crDims[1]/dims[1] * crLen[1],
		     (float)crDims[2]/dims[2] * crLen[2]);
  m_bbox[0] = m_bbox[0] + crsLen;
  m_bbox[1] = m_bbox[0] + crdLen;

  unsigned char *datap = (unsigned char*)m_data;
  unsigned char buff[256];
  register int k, j;

  // skip crStart[2] x dims[0] x dims[1]
  if ( fseek(fp, dims[0]*dims[1]*crStart[2], SEEK_CUR) < 0 ) {
    fclose(fp);
    return false;
  }

  // reading
  for ( k = 0; k < crDims[2]; k++ ) {
    // skip dims[0] x crStart[1]
    if ( fseek(fp, dims[0]*crStart[1], SEEK_CUR) < 0 ) {
      fclose(fp);
      return false;
    }

    for ( j = 0; j < crDims[1]; j++ ) {
      // read into buff
      if ( fread(buff, 1, dims[0], fp) < dims[0] ) {
	fclose(fp);
	return false;
      }

      // copy to datap
      memcpy(datap, &buff[crStart[0]], crDims[0]);

      // move datap
      datap += crDims[0];
    } // end of for(j)

    // skip dims[0] x (dims[1] - crEnd[1])
    if ( fseek(fp, dims[0]*(dims[1]-crEnd[1]), SEEK_CUR) < 0 ) {
      fclose(fp);
      return false;
    }
  } // end of for(k)

  fclose(fp);

  m_offset = crStart;
  m_orgDims = dims;
  return true;
}

// STATIC
Dim3 CropReadVolume::GetWholeSizeAvs(const char* path)
{
  if ( ! path ) return Dim3(0,0,0);
  FILE* fp = fopen(path, "rb");
  if ( ! fp ) return Dim3(0,0,0);

  // dimension check
  unsigned char xdim[3];
  fread(xdim, 1, 3, fp);
  fclose(fp);
  return Dim3(xdim[0], xdim[1], xdim[2]);
}


bool CropReadVolume::CropLoadFdvVol(const char* path,
				    const Dim3& crStart, const Dim3& xcrDims,
				    const size_t stp)
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
  if ( mtf == CES::UnKnown ) {fclose(fp); return false;}
  if ( mtf == CES::UnMatch ) doBx = true;

  // read dims
  int xdim[5];
  if ( fread(xdim, sizeof(int), 5, fp) < 5 ) {fclose(fp); return false;}
  if ( doBx ) BSWAPVEC(xdim, 5);
  if ( xdim[1] < 1 || xdim[2] < 1 || xdim[3] < 1 ) {fclose(fp); return false;}
  Dim3 dims(xdim[1], xdim[2], xdim[3]);

  Dim3 crDims(xcrDims);
  if ( xcrDims.Size() < 1 )
    crDims = Dim3(dims[0]-crStart[0], dims[1]-crStart[1], dims[2]-crStart[2]);
  if ( crDims.Size() < 1 ) {
    fclose(fp);
    return false;
  }

  Dim3 crEnd(crStart[0]+crDims[0], crStart[1]+crDims[1], crStart[2]+crDims[2]);
  if ( crEnd[0] > dims[0] || crEnd[1] > dims[1] || crEnd[2] > dims[2] ) {
    fclose(fp);
    return false;
  }

  // allocation
  m_format = UNSIGNED_BYTE;
  if ( ! AllocData(crDims) ) {fclose(fp); return false;}

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
  m_orgBbox[0] = m_bbox[0];
  m_orgBbox[1] = m_bbox[1];
  Vec3<float> crLen = m_bbox[1] - m_bbox[0];
  Vec3<float> crsLen((float)crStart[0]/dims[0] * crLen[0],
		     (float)crStart[1]/dims[1] * crLen[1],
		     (float)crStart[2]/dims[2] * crLen[2]);
  Vec3<float> crdLen((float)crDims[0]/dims[0] * crLen[0],
		     (float)crDims[1]/dims[1] * crLen[1],
		     (float)crDims[2]/dims[2] * crLen[2]);
  m_bbox[0] = m_bbox[0] + crsLen;
  m_bbox[1] = m_bbox[0] + crdLen;  

  unsigned char *datap = (unsigned char*)m_data;
  unsigned char *buff = new unsigned char[dims[0]];
  if ( ! buff ) {fclose(fp); return false;}
  register int k, j;

  // skip sz
  if ( fread(&sz, sizeof(int), 1, fp) < 1 ) {fclose(fp); return false;}
  if ( doBx ) BSWAP32(sz);
  if ( sz != sz4 ) {fclose(fp); return false;}

  // skip crStart[2] x dims[0] x dims[1]
  if ( fseek(fp, dims[0]*dims[1]*crStart[2], SEEK_CUR) < 0 ) {
    delete [] buff; fclose(fp); return false;}

  // reading
  for ( k = 0; k < crDims[2]; k++ ) {
    // skip dims[0] x crStart[1]
    if ( fseek(fp, dims[0]*crStart[1], SEEK_CUR) < 0 ) {
      delete [] buff; fclose(fp); return false;}

    for ( j = 0; j < crDims[1]; j++ ) {
      // read into buff
      if ( fread(buff, 1, dims[0], fp) < dims[0] ) {
	delete [] buff; fclose(fp); return false;}

      // copy to datap
      memcpy(datap, &buff[crStart[0]], crDims[0]);

      // move datap
      datap += crDims[0];
    } // end of for(j)

    // skip dims[0] x (dims[1] - crEnd[1])
    if ( fseek(fp, dims[0]*(dims[1]-crEnd[1]), SEEK_CUR) < 0 ) {
      delete [] buff; fclose(fp); return false;}
  } // end of for(k)

  fclose(fp);
  delete [] buff;

  m_offset = crStart;
  m_orgDims = dims;
  return true;
}

// STATIC
Dim3 CropReadVolume::GetWholeSizeFdv(const char* path, size_t* pSteps)
{
  Dim3 dims(0,0,0);

  if ( ! path ) return dims;
  FILE* fp = fopen(path, "rb");
  if ( ! fp ) return dims;

  // get file status for file-length
  struct stat st_buf;
  if ( fstat(fileno(fp), &st_buf) < 0 ) {fclose(fp); return dims;}

  // endian check
  bool doBx = false;
  CES::EMatchType mtf = CES::MatchEndian(fp, 12);
  if ( mtf == CES::UnKnown ) {fclose(fp); return false;}
  if ( mtf == CES::UnMatch ) doBx = true;

  // read dims
  int xdim[5];
  if ( fread(xdim, sizeof(int), 5, fp) < 5 ) {fclose(fp); return dims;}
  if ( doBx ) BSWAPVEC(xdim, 5);
  if ( xdim[1] < 1 || xdim[2] < 1 || xdim[3] < 1 ) {fclose(fp); return dims;}

  fclose(fp);
  dims = Dim3(xdim[1], xdim[2], xdim[3]);
  if ( ! pSteps ) return dims;

  size_t sz = dims.Size();
  size_t sz4 = (sz%4==0 ? sz : sz+4-sz%4); // data size with padding
  size_t stpLen = (4 + 8 + 2)*4 + sz4;
  *pSteps = (st_buf.st_size - 5*4) / stpLen; // #of steps
  return dims;
}

// STATIC
bool CropReadVolume::GetWholeBboxFdv(Vec3<float>* bbox,
                                     const char* path, const size_t step)
{
  if ( ! bbox || ! path ) return false;

  FILE* fp = fopen(path, "rb");
  if ( ! fp ) return false;

  // get file status for file-length
  struct stat st_buf;
  if ( fstat(fileno(fp), &st_buf) < 0 ) {fclose(fp); return false;}

  // endian check
  bool doBx = false;
  CES::EMatchType mtf = CES::MatchEndian(fp, 12);
  if ( mtf == CES::UnKnown ) {fclose(fp); return false;}
  if ( mtf == CES::UnMatch ) doBx = true;

  // read dims
  int xdim[5];
  if ( fread(xdim, sizeof(int), 5, fp) < 5 ) {fclose(fp); return false;}
  if ( doBx ) BSWAPVEC(xdim, 5);
  if ( xdim[1] < 1 || xdim[2] < 1 || xdim[3] < 1 ) {fclose(fp); return false;}

  Dim3 dims = Dim3(xdim[1], xdim[2], xdim[3]);
  size_t sz = dims.Size();
  size_t sz4 = (sz%4==0 ? sz : sz+4-sz%4); // data size with padding
  size_t stpLen = (4 + 8 + 2)*4 + sz4;
  sz = (st_buf.st_size - 5*4) / stpLen; // #of steps
  if ( step >= sz ) {fclose(fp); return false;}

  // skip datas
  for ( size_t i = 0; i < step; i++ ) {
    if ( fseek(fp, stpLen, SEEK_CUR) != 0 ) {fclose(fp); return false;}
  }
  if ( fseek(fp, 5*4, SEEK_CUR) != 0 ) {fclose(fp); return false;}

  // read bbox
  float bbuff[6];
  if ( fread(bbuff, sizeof(float), 6, fp) < 6 ) {fclose(fp); return false;}
  if ( doBx ) BSWAPVEC(bbuff, 6);

  fclose(fp);

  bbox[0] = Vec3<float>(bbuff[0], bbuff[1], bbuff[2]);
  bbox[1] = Vec3<float>(bbuff[3], bbuff[4], bbuff[5]);
  return true;
}

