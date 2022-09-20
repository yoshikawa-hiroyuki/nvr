/*
 * 4DVis - 4D Visualization system of RIKEN -
 * Copyright (c) 2000-2005, RIKEN, Japan, All right reserved.
 */

#ifndef _NVR_BRICK_SORTER_H_
#define _NVR_BRICK_SORTER_H_

#include <deque>
#include <algorithm>
#include <functional>
#include "nvrBrick.h"


//! ボリュームサブブロックのソータークラス
/*! ボリュームサブブロックのポインタリストに対し、指定された条件で
    並び換えを行う機能を実装しているクラスです。
*/
class nvrBrickSorter {
public:
  //! 誤差に基づくボリュームサブブロックのソート
  /*! オリジナルボリュームとの誤差に基づいて、ボリュームサブブロックの
      ソートを行います。
      eltで指定された誤差評価タイプがNVR::TEST_LEVELの場合、各サブブロックの
      テストレベルにおける誤差を用いてソートします。
      誤差評価タイプがNVR::REDUCEDの場合は、各サブブロックで実際に解像度縮小
      されている縮小度レベルでの誤差を用いてソートします。
  */
  static void SortByError(std::deque<nvrBrick*>&,
                          const NVR::ErrLevelType elt =NVR::TEST_LEVEL);

  //! 優先度に基づくボリュームサブブロックのソート
  static void SortByPriority(std::deque<nvrBrick*>&);

private:
  // テストレベルを参照する誤差比較関数
  static bool ErrCompTest(const nvrBrick* b1, const nvrBrick* b2);
  // 現在の縮小度を参照する誤差比較関数
  static bool ErrCompCurrent(const nvrBrick* b1, const nvrBrick* b2);
  // 優先度を参照する比較関数
  static bool PrioComp(const nvrBrick* b1, const nvrBrick* b2);
};


//-----------------------------------------------------------
//  class nvrBrickSorter : all methods are STATIC
//-----------------------------------------------------------

inline bool
nvrBrickSorter::ErrCompTest(const nvrBrick* b1, const nvrBrick* b2) {
  if ( ! b1 ) return false;
  if ( ! b2 ) return (b1 != NULL);
  float e1 = b1->GetResidualErr(b1->GetTestLevel() +1);
  float e2 = b2->GetResidualErr(b2->GetTestLevel() +1);
  return e1 < e2;
}

inline bool
nvrBrickSorter::ErrCompCurrent(const nvrBrick* b1, const nvrBrick* b2) {
  if ( ! b1 ) return false;
  if ( ! b2 ) return (b1 != NULL);
  float e1 = b1->GetResidualErr(b1->GetReduceLevel() +1);
  float e2 = b2->GetResidualErr(b2->GetReduceLevel() +1);
  return e1 < e2;
}

inline bool
nvrBrickSorter::PrioComp(const nvrBrick* b1, const nvrBrick* b2) {
  if ( ! b2 ) return false;
  if ( ! b1 ) return (b2 != NULL);
  return (b1->GetPrior() > b2->GetPrior());
}

#endif // _NVR_BRICK_SORTER_H_
