//
// CropReadVolume
//
#ifndef _CROP_READ_VOLUME_H_
#define _CROP_READ_VOLUME_H_

#include "SimpleVolume.h"


class CropReadVolume : public SimpleVolume {
public:
  CropReadVolume() : SimpleVolume() {}
  virtual ~CropReadVolume() {}

  virtual bool LoadVoxVol(const char* path) {
    if ( ! SimpleVolume::LoadVoxVol(path) ) return false;
    m_orgBbox[0] = m_bbox[0]; m_orgBbox[1] = m_bbox[1];
    return true;
  }
  virtual bool LoadAvsVol(const char* path) {
    if ( ! SimpleVolume::LoadAvsVol(path) ) return false;
    m_orgBbox[0] = m_bbox[0]; m_orgBbox[1] = m_bbox[1];
    return true;
  }
  virtual bool LoadFdvVol(const char* path, const size_t stp =0) {
    if ( ! SimpleVolume::LoadFdvVol(path, stp) ) return false;
    m_orgBbox[0] = m_bbox[0]; m_orgBbox[1] = m_bbox[1];
    return true;
  }

  bool CropLoadVoxVol(const char* path,
		      const NVR::Dim3& crStart =NVR::Dim3(0,0,0),
		      const NVR::Dim3& crDims =NVR::Dim3(0,0,0));
  bool CropLoadAvsVol(const char* path,
		      const NVR::Dim3& crStart =NVR::Dim3(0,0,0),
		      const NVR::Dim3& crDims =NVR::Dim3(0,0,0));
  bool CropLoadFdvVol(const char* path,
		      const NVR::Dim3& crStart =NVR::Dim3(0,0,0),
		      const NVR::Dim3& crDims =NVR::Dim3(0,0,0),
		      const size_t =0);

  NVR::Dim3 GetOffset() const {return m_offset;}
  NVR::Dim3 GetOrgDims() const {return m_orgDims;}
  const CES::Vec3<float>* GetOrgBbox() const {return m_orgBbox;}

  static NVR::Dim3 GetWholeSizeVox(const char* path);
  static NVR::Dim3 GetWholeSizeAvs(const char* path);
  static NVR::Dim3 GetWholeSizeFdv(const char* path, size_t* pSteps =NULL);
  static bool GetWholeBboxFdv(CES::Vec3<float>* bbox,
                              const char* path, const size_t step =0);

protected:
  mutable NVR::Dim3 m_offset;
  mutable NVR::Dim3 m_orgDims;
  mutable CES::Vec3<float> m_orgBbox[2];
};

#endif // _CROP_READ_VOLUME_H_
