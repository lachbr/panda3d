// Filename: hardwareChannel.h
// Created by:  mike (09Jan97)
//
////////////////////////////////////////////////////////////////////
//
#ifndef HARDWARECHANNEL_H
#define HARDWARECHANNEL_H
//
////////////////////////////////////////////////////////////////////
// Includes
////////////////////////////////////////////////////////////////////
#include <pandabase.h>

#include "graphicsChannel.h"

////////////////////////////////////////////////////////////////////
// Defines
////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////
//       Class : HardwareChannel
// Description : Video output channels if available on the current
//		 platform
//		 NOTE: hardware channels belong to a pipe rather
//		 than to a particular window
////////////////////////////////////////////////////////////////////
class EXPCL_PANDA HardwareChannel : public GraphicsChannel
{
    public:

    	HardwareChannel( GraphicsWindow* window );
	~HardwareChannel( void );

        virtual void window_resized(int x, int y);

	INLINE int get_id( void ) const;
	INLINE int get_xorg( void ) const;
	INLINE int get_yorg( void ) const;
	INLINE int get_xsize( void ) const;
	INLINE int get_ysize( void ) const;

    protected:

	int				_id;
	int				_xorg;
	int				_yorg;
	int 				_xsize;
	int				_ysize;

    public:

        static TypeHandle get_class_type() {
            return _type_handle;
        }
        static void init_type() {
            GraphicsChannel::init_type();
            register_type(_type_handle, "HardwareChannel",
                          GraphicsChannel::get_class_type());
        }
        virtual TypeHandle get_type() const {
            return get_class_type();
        }
        virtual TypeHandle force_init_type() {init_type(); return get_class_type();}
 
    private:

        static TypeHandle _type_handle;
};

#include "hardwareChannel.I"

#endif
