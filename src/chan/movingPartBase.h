// Filename: movingPartBase.h
// Created by:  drose (22Feb99)
//
////////////////////////////////////////////////////////////////////

#ifndef MOVINGPARTBASE_H
#define MOVINGPARTBASE_H

#include <pandabase.h>

#include "partGroup.h"
#include "partBundle.h"
#include "animChannelBase.h"

////////////////////////////////////////////////////////////////////
//       Class : MovingPartBase
// Description : This is the base class for a single animatable piece
//               that may be bound to one channel (or more, if
//               blending is in effect).  It corresponds to, for
//               instance, a single joint or slider of a character.
//
//               MovingPartBase does not have a particular value type.
//               See the derived template class, MovingPart, for this.
////////////////////////////////////////////////////////////////////
class EXPCL_PANDA MovingPartBase : public PartGroup {
protected:
  INLINE MovingPartBase(const MovingPartBase &copy);

public:
  MovingPartBase(PartGroup *parent, const string &name);

  virtual TypeHandle get_value_type() const=0;
  virtual AnimChannelBase *make_initial_channel() const=0;
  virtual void write(ostream &out, int indent_level) const;
  virtual void write_with_value(ostream &out, int indent_level) const;
  virtual void output_value(ostream &out) const=0;

  virtual void do_update(PartBundle *root, PartGroup *parent,
                         bool parent_changed, bool anim_changed);

  virtual void get_blend_value(const PartBundle *root)=0;
  virtual void update_internals(PartGroup *parent, bool self_changed,
                                bool parent_changed);

protected:
  MovingPartBase(void);

  virtual void pick_channel_index(list<int> &holes, int &next) const;
  virtual void bind_hierarchy(AnimGroup *anim, int channel_index);

  typedef vector< PT(AnimChannelBase) > Channels;
  Channels _channels;

public:

  virtual TypeHandle get_type() const {
    return get_class_type();
  }
  virtual TypeHandle force_init_type() {init_type(); return get_class_type();}
PUBLISHED:
  static TypeHandle get_class_type() {
    return _type_handle;
  }
public:
  static void init_type() {
    PartGroup::init_type();
    register_type(_type_handle, "MovingPartBase",
                  PartGroup::get_class_type());
  }

private:
  static TypeHandle _type_handle;
};

#include "movingPartBase.I"

#endif


 

