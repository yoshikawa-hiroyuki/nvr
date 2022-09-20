//
// SimpleVolume
//
#ifndef _SIMPLE_VOLUME_H_
#define _SIMPLE_VOLUME_H_

#include <stdio.h>
#include <string.h>
#ifdef unix
#include <unistd.h>
#endif
#include "utilMemory.h"
#include "utilEndian.h"
#include "utilMath.h"

#include "nvrDefs.h"

struct SimpleVolume {
  NVR::Dim3 m_dims;
  NVR::DataFormat m_format;
  void* m_data;
  CES::Vec3<float> m_bbox[2];

  SimpleVolume() : m_format(NVR::UNSIGNED_BYTE), m_data(NULL) {}
  virtual ~SimpleVolume() {
    if ( m_data ) {
      CES::DeAllocate(m_data);
      m_data = NULL;
    }
  }

  virtual bool AllocData(const NVR::Dim3& dims);
  virtual bool LoadVoxVol(const char* path);
  virtual bool LoadAvsVol(const char* path);
  virtual bool LoadFdvVol(const char* path, const size_t =0);
  virtual bool EnPower2();

  template<class T> bool copyData(const NVR::Dim3 newDims, T* newData);
};

#endif //  _SIMPLE_VOLUME_H_
