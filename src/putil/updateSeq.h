// Filename: updateSeq.h
// Created by:  drose (30Sep99)
//
////////////////////////////////////////////////////////////////////

#ifndef UPDATE_SEQ
#define UPDATE_SEQ

#include <pandabase.h>

////////////////////////////////////////////////////////////////////
// 	 Class : UpdateSeq
// Description : This is a sequence number that increments
//               monotonically.  It can be used to track cache
//               updates, or serve as a kind of timestamp for any
//               changing properties.
//
//               A special class is used instead of simply an int, so
//               we can elegantly handle such things as wraparound and
//               special cases.  There are two special cases.
//               Firstly, a sequence number is 'initial' when it is
//               first created.  This sequence is older than any other
//               sequence number.  Secondly, a sequence number may be
//               explicitly set to 'old'.  This is older than any
//               other sequence number except 'initial'.  Finally, we
//               have the explicit number 'fresh', which is newer
//               than any other sequence number.  All other sequences
//               are numeric and are monotonically increasing.
////////////////////////////////////////////////////////////////////
class EXPCL_PANDA UpdateSeq {
public:
  INLINE UpdateSeq();
  INLINE static UpdateSeq initial();
  INLINE static UpdateSeq old();
  INLINE static UpdateSeq fresh();

  INLINE UpdateSeq(const UpdateSeq &copy);
  INLINE UpdateSeq &operator = (const UpdateSeq &copy);

  INLINE void clear();

  INLINE bool is_initial() const;
  INLINE bool is_old() const;
  INLINE bool is_fresh() const;
  INLINE bool is_special() const;

  INLINE bool operator == (const UpdateSeq &other) const;
  INLINE bool operator != (const UpdateSeq &other) const;
  INLINE bool operator < (const UpdateSeq &other) const;
  INLINE bool operator <= (const UpdateSeq &other) const;

  INLINE UpdateSeq operator ++ ();
  INLINE UpdateSeq operator ++ (int);

  INLINE void output(ostream &out) const;

private:
  enum SpecialCases {
    SC_initial = 0,
    SC_old = 1,
    SC_fresh = ~0,
  };

  unsigned int _seq;
};

INLINE ostream &operator << (ostream &out, const UpdateSeq &value);

#include "updateSeq.I"

#endif
