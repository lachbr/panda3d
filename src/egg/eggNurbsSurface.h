// Filename: eggNurbsSurface.h
// Created by:  drose (15Feb00)
//
////////////////////////////////////////////////////////////////////

#ifndef EGGNURBSSURFACE_H
#define EGGNURBSSURFACE_H

#include <pandabase.h>

#include "eggSurface.h"
#include "eggNurbsCurve.h"

#include <vector_double.h>

#include <list>

////////////////////////////////////////////////////////////////////
//       Class : EggNurbsSurface
// Description : A parametric NURBS surface.
////////////////////////////////////////////////////////////////////
class EXPCL_PANDAEGG EggNurbsSurface : public EggSurface {
public:
  typedef list< PT(EggNurbsCurve) > Curves;
  typedef Curves Loop;
  typedef list<Loop> Loops;
  typedef Loops Trim;
  typedef list<Trim> Trims;

  INLINE EggNurbsSurface(const string &name = "");
  INLINE EggNurbsSurface(const EggNurbsSurface &copy);
  INLINE EggNurbsSurface &operator = (const EggNurbsSurface &copy);

  void setup(int u_order, int v_order,
             int num_u_knots, int num_v_knots);

  INLINE void set_u_order(int u_order);
  INLINE void set_v_order(int v_order);
  void set_num_u_knots(int num);
  void set_num_v_knots(int num);

  INLINE void set_u_knot(int k, double value);
  INLINE void set_v_knot(int k, double value);
  INLINE void set_cv(int ui, int vi, EggVertex *vertex);

  bool is_valid() const;

  INLINE int get_u_order() const;
  INLINE int get_v_order() const;
  INLINE int get_u_degree() const;
  INLINE int get_v_degree() const;
  INLINE int get_num_u_knots() const;
  INLINE int get_num_v_knots() const;
  INLINE int get_num_u_cvs() const;
  INLINE int get_num_v_cvs() const;
  INLINE int get_num_cvs() const;

  INLINE int get_u_index(int vertex_index) const;
  INLINE int get_v_index(int vertex_index) const;
  INLINE int get_vertex_index(int ui, int vi) const;

  bool is_closed_u() const;
  bool is_closed_v() const;

  INLINE double get_u_knot(int k) const;
  INLINE double get_v_knot(int k) const;
  INLINE EggVertex *get_cv(int ui, int vi) const;

  virtual void write(ostream &out, int indent_level) const;

  Curves _curves_on_surface;
  Trims _trims;

protected:
  virtual void r_apply_texmats(EggTextureCollection &textures);

private:
  typedef vector_double Knots;
  Knots _u_knots;
  Knots _v_knots;
  int _u_order;
  int _v_order;

public:

  static TypeHandle get_class_type() {
    return _type_handle;
  }
  static void init_type() {
    EggPrimitive::init_type();
    register_type(_type_handle, "EggNurbsSurface",
                  EggPrimitive::get_class_type());
  }
  virtual TypeHandle get_type() const {
    return get_class_type();
  }
  virtual TypeHandle force_init_type() {init_type(); return get_class_type();}
 
private:
  static TypeHandle _type_handle;
 
};

#include "eggNurbsSurface.I"

#endif
