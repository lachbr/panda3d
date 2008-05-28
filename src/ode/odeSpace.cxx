// Filename: odeSpace.cxx
// Created by:  joswilso (27Dec06)
//
////////////////////////////////////////////////////////////////////
//
// PANDA 3D SOFTWARE
// Copyright (c) Carnegie Mellon University.  All rights reserved.
//
// All use of this software is subject to the terms of the revised BSD
// license.  You should have received a copy of this license along
// with this source code in a file named "LICENSE."
//
////////////////////////////////////////////////////////////////////

#include "config_ode.h"
#include "odeSpace.h"


TypeHandle OdeSpace::_type_handle;
// this data is used in auto_collide
const int OdeSpace::MAX_CONTACTS = 16; 
OdeWorld* OdeSpace::_collide_world; 
OdeSpace* OdeSpace::_collide_space; 
dJointGroupID OdeSpace::_collide_joint_group; 
int OdeSpace::contactCount = 0;
double OdeSpace::contact_data[192];
int OdeSpace::contact_ids[128];

OdeSpace::
OdeSpace(dSpaceID id) : 
  _id(id) {
}

OdeSpace::
~OdeSpace() {
}

void OdeSpace::
destroy() {
  dSpaceDestroy(_id);
}

int OdeSpace::
query(const OdeGeom& geom) const {
  return dSpaceQuery(_id, geom.get_id());
}

int OdeSpace::
query(const OdeSpace& space) const {
  return dSpaceQuery(_id, (dGeomID)space.get_id());
}

void OdeSpace::
add(OdeSpace& space) {
  dSpaceAdd(_id, (dGeomID)space.get_id());
}

void OdeSpace::
remove(OdeSpace& space) {
  dSpaceRemove(_id, (dGeomID)space.get_id());
}

void OdeSpace::
add(OdeGeom& geom) {
  dSpaceAdd(_id, geom.get_id());
}

void OdeSpace::
remove(OdeGeom& geom) {
  dSpaceRemove(_id, geom.get_id());
}

void OdeSpace::
clean() {
  dSpaceClean(_id);
}

OdeGeom  OdeSpace::
get_geom(int i) {
  return OdeGeom(dSpaceGetGeom(_id, i));
}


void OdeSpace::
write(ostream &out, unsigned int indent) const {
  #ifndef NDEBUG //[
  out.width(indent); out << "" << get_type() << "(id = " << _id << ")";
  #endif //] NDEBUG
}


void OdeSpace::
set_auto_collide_world(OdeWorld &world)
{
    my_world = &world;
}

void OdeSpace::
set_auto_collide_joint_group(OdeJointGroup &joint_group)
{
    _collide_joint_group = joint_group.get_id();
}

int OdeSpace::
autoCollide()
{
    OdeSpace::contactCount = 0;
    _collide_space = this;
    _collide_world = my_world;
    dSpaceCollide(_id, this, &autoCallback);
    return OdeSpace::contactCount;
}

double OdeSpace:: 
get_contact_data(int data_index)
// get the contact data it looks like so [x1,y1,z1,x2,y2,z2... x64,y64,z64]
// use the return in from autoCollide to determine how much of the data is
// valid. The data would be more straight forward but the callbacks have to be
// static.
{
    return OdeSpace::contact_data[data_index];
}

int OdeSpace:: 
get_contact_id(int data_index, int first)
// get the contact data it looks like so [x1,y1,z1,x2,y2,z2... x64,y64,z64]
// use the return in from autoCollide to determine how much of the data is
// valid. The data would be more straight forward but the callbacks have to be
// static.
{
    if (first == 0)
    {
        return OdeSpace::contact_ids[(data_index * 2) + 0];
    }
    else
    {
        return OdeSpace::contact_ids[(data_index * 2) + 1];
    }
}




void OdeSpace::
autoCallback(void *data, dGeomID o1, dGeomID o2)
// uses data stored on the world to resolve collisions so you don't have to use near_callbacks in python
{
    int i;
    static int autoCallbackCounter = 0;
    dBodyID b1 = dGeomGetBody(o1);
    dBodyID b2 = dGeomGetBody(o2);

    dContact contact[OdeSpace::MAX_CONTACTS];
        
    int surface1 = _collide_space->get_surface_type(o1);
    int surface2 = _collide_space->get_surface_type(o2);
    
    sSurfaceParams collide_params;
    collide_params = _collide_world->get_surface(surface1, surface2);
    
    for (i=0; i < OdeSpace::MAX_CONTACTS; i++)
    {
        contact[i].surface.mode = collide_params.colparams.mode; 
        contact[i].surface.mu = collide_params.colparams.mu; 
        contact[i].surface.mu2 = collide_params.colparams.mu2; 
        contact[i].surface.bounce = collide_params.colparams.bounce; 
        contact[i].surface.bounce_vel = collide_params.colparams.bounce_vel; 
        contact[i].surface.soft_cfm = collide_params.colparams.soft_cfm; 
    }
    
    static int numc = 0;
    numc = dCollide(o1, o2, OdeSpace::MAX_CONTACTS, &contact[0].geom, sizeof(dContact));

    if (numc)
    {
        if (odespace_cat.is_debug() && (autoCallbackCounter%30 == 0)) {
            odespace_cat.debug() << autoCallbackCounter <<" collision between geoms " << o1 << " and " << o2 << "\n";
            odespace_cat.debug() << "collision between body " << b1 << " and " << b2 << "\n";
            odespace_cat.debug() << "surface1= "<< surface1 << " surface2=" << surface2 << "\n";
        }
        autoCallbackCounter += 1;

        for(i=0; i < numc; i++)
        {
            dJointID c = dJointCreateContact(_collide_world->get_id(), _collide_joint_group, contact + i);
            if ((_collide_space->get_collide_id(o1) >= 0) && (_collide_space->get_collide_id(o2) >= 0))
            {
                dJointAttach(c, b1, b2);
            }
            // this creates contact position data for python. It is useful for debugging only 64 points are stored
            if(contactCount < 64)
            {
                OdeSpace::contact_data[0 + (OdeSpace::contactCount * 3)] = contact[i].geom.pos[0];
                OdeSpace::contact_data[1 + (OdeSpace::contactCount * 3)] = contact[i].geom.pos[1];
                OdeSpace::contact_data[2 + (OdeSpace::contactCount * 3)] = contact[i].geom.pos[2];
                OdeSpace::contact_ids[0 + (OdeSpace::contactCount * 2)] = _collide_space->get_collide_id(o1);
                OdeSpace::contact_ids[1 + (OdeSpace::contactCount * 2)] = _collide_space->get_collide_id(o2);
                OdeSpace::contactCount += 1;
            }
        }
        _collide_world->set_dampen_on_bodies(b1, b2, collide_params.dampen);
    }

}

OdeSimpleSpace OdeSpace::
convert_to_simple_space() const {
  nassertr(_id != 0, OdeSimpleSpace((dSpaceID)0));
  nassertr(get_class() == OdeGeom::GC_simple_space, OdeSimpleSpace((dSpaceID)0));
  return OdeSimpleSpace(_id);
}

OdeHashSpace OdeSpace::
convert_to_hash_space() const {
  nassertr(_id != 0, OdeHashSpace((dSpaceID)0));
  nassertr(get_class() == OdeGeom::GC_hash_space, OdeHashSpace((dSpaceID)0));
  return OdeHashSpace(_id);
}

OdeQuadTreeSpace OdeSpace::
convert_to_quad_tree_space() const {
  nassertr(_id != 0, OdeQuadTreeSpace((dSpaceID)0));
  nassertr(get_class() == OdeGeom::GC_quad_tree_space, OdeQuadTreeSpace((dSpaceID)0));
  return OdeQuadTreeSpace(_id);
}


void OdeSpace::
set_surface_type( int surfaceType, dGeomID id){
  _geom_surface_map[id]= surfaceType;
}

void OdeSpace::
set_surface_type(OdeGeom& geom,  int surfaceType){
  dGeomID id = geom.get_id();
  set_surface_type(surfaceType, id);
}

int OdeSpace::
get_surface_type(dGeomID id)
{
  GeomSurfaceMap::iterator iter = _geom_surface_map.find(id);
  if (iter != _geom_surface_map.end()) {
    // odespace_cat.debug() << "get_default_surface_type the geomId =" << id <<" surfaceType=" << iter->second << "\n";
    return iter->second;
  }
  // odespace_cat.debug() << "get_default_surface_type not in map, returning 0" ;
  return 0;
}

int OdeSpace::
get_surface_type(OdeGeom& geom){
  dGeomID id = geom.get_id();
  return get_surface_type(id);
}


int OdeSpace::
set_collide_id( int collide_id, dGeomID id){
  _geom_collide_id_map[id]= collide_id;

  //odespace_cat.debug() << "set_collide_id " << id << " " << _geom_collide_id_map[id] <<"\n";
    
/*
  GeomCollideIdMap::iterator iter2 = _geom_collide_id_map.begin();
  while (iter2 != _geom_collide_id_map.end()) {
    odespace_cat.debug() << "set print get_collide_id the geomId =" << iter2->first <<" collide_id=" << iter2->second << "\n";
    iter2++;    
  }
  
  get_collide_id(id);
*/
  return _geom_collide_id_map[id];
}

int OdeSpace::
set_collide_id( OdeGeom& geom, int collide_id)
{
    dGeomID id = geom.get_id();
    return set_collide_id(collide_id, id);
}

int OdeSpace::
get_collide_id(OdeGeom& geom)
{
    dGeomID id = geom.get_id();
    return get_collide_id(id);
}


int OdeSpace::
get_collide_id(dGeomID id)
{

/*
  GeomCollideIdMap::iterator iter2 = _geom_collide_id_map.begin();
  while (iter2 != _geom_collide_id_map.end()) {
    odespace_cat.debug() << "get print get_collide_id the geomId =" << iter2->first <<" collide_id=" << iter2->second << "\n";
    iter2++;    
  }
*/
  
  GeomCollideIdMap::iterator iter = _geom_collide_id_map.find(id);
  if (iter != _geom_collide_id_map.end()) {
    //odespace_cat.debug() << "get_collide_id the geomId =" << id <<" collide_id=" << iter->second << "\n";
    return iter->second;
  }

  //odespace_cat.debug() << "get_collide_id not in map, returning 0 id =" << id << "\n" ;
  return 0;
}

