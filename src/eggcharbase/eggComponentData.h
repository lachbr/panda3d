// Filename: eggComponentData.h
// Created by:  drose (26Feb01)
// 
////////////////////////////////////////////////////////////////////

#ifndef EGGCOMPONENTDATA_H
#define EGGCOMPONENTDATA_H

#include <pandatoolbase.h>

#include <namable.h>

class EggCharacterCollection;
class EggCharacterData;
class EggBackPointer;
class EggObject;

////////////////////////////////////////////////////////////////////
//       Class : EggComponentData
// Description : This is the base class of both EggJointData and
//               EggSliderData.  It represents a single component of a
//               character, either a joint or a slider, along with
//               back pointers to the references to this component in
//               all model and animation egg files read.
////////////////////////////////////////////////////////////////////
class EggComponentData : public Namable {
public:
  EggComponentData(EggCharacterCollection *collection,
                   EggCharacterData *char_data);
  virtual ~EggComponentData();

  void add_name(const string &name);
  bool matches_name(const string &name) const;

  virtual void add_back_pointer(int model_index, EggObject *egg_object)=0;
  virtual void write(ostream &out, int indent_level = 0) const=0;

  INLINE int get_num_models() const;
  INLINE bool has_model(int model_index) const;
  INLINE EggBackPointer *get_model(int model_index) const;
  void set_model(int model_index, EggBackPointer *back);

protected:

  // This points back to all the egg structures that reference this
  // particular table or slider.
  typedef vector<EggBackPointer *> BackPointers;
  BackPointers _back_pointers;


  EggCharacterCollection *_collection;
  EggCharacterData *_char_data;
};

#include "eggComponentData.I"

#endif


