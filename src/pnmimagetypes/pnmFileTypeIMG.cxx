// Filename: pnmFileTypeIMG.cxx
// Created by:  drose (19Jun00)
// 
////////////////////////////////////////////////////////////////////

#include "pnmFileTypeIMG.h"
#include "config_pnmimagetypes.h"

// Since raw image files don't have a magic number, we'll make a little
// sanity check on the size of the image.  If either the width or height is
// larger than this, it must be bogus.
#define INSANE_SIZE 20000

static const char * const extensions[] = {
  "img"
};
static const int num_extensions = sizeof(extensions) / sizeof(const char *);

TypeHandle PNMFileTypeIMG::_type_handle;

////////////////////////////////////////////////////////////////////
//     Function: PNMFileTypeIMG::Constructor
//       Access: Public
//  Description: 
////////////////////////////////////////////////////////////////////
PNMFileTypeIMG::
PNMFileTypeIMG() {
}

////////////////////////////////////////////////////////////////////
//     Function: PNMFileTypeIMG::get_name
//       Access: Public, Virtual
//  Description: Returns a few words describing the file type.
////////////////////////////////////////////////////////////////////
string PNMFileTypeIMG::
get_name() const {
  return "Raw binary RGB";
}

////////////////////////////////////////////////////////////////////
//     Function: PNMFileTypeIMG::get_num_extensions
//       Access: Public, Virtual
//  Description: Returns the number of different possible filename
//               extensions associated with this particular file type.
////////////////////////////////////////////////////////////////////
int PNMFileTypeIMG::
get_num_extensions() const {
  return num_extensions;
}

////////////////////////////////////////////////////////////////////
//     Function: PNMFileTypeIMG::get_extension
//       Access: Public, Virtual
//  Description: Returns the nth possible filename extension
//               associated with this particular file type, without a
//               leading dot.
////////////////////////////////////////////////////////////////////
string PNMFileTypeIMG::
get_extension(int n) const {
  nassertr(n >= 0 && n < num_extensions, string());
  return extensions[n];
}

////////////////////////////////////////////////////////////////////
//     Function: PNMFileTypeIMG::get_suggested_extension
//       Access: Public, Virtual
//  Description: Returns a suitable filename extension (without a
//               leading dot) to suggest for files of this type, or
//               empty string if no suggestions are available.
////////////////////////////////////////////////////////////////////
string PNMFileTypeIMG::
get_suggested_extension() const {
  return "img";
}

////////////////////////////////////////////////////////////////////
//     Function: PNMFileTypeIMG::make_reader
//       Access: Public, Virtual
//  Description: Allocates and returns a new PNMReader suitable for
//               reading from this file type, if possible.  If reading
//               from this file type is not supported, returns NULL.
////////////////////////////////////////////////////////////////////
PNMReader *PNMFileTypeIMG::
make_reader(FILE *file, bool owns_file, const string &magic_number) {
  init_pnm();
  return new Reader(this, file, owns_file, magic_number);
}

////////////////////////////////////////////////////////////////////
//     Function: PNMFileTypeIMG::make_writer
//       Access: Public, Virtual
//  Description: Allocates and returns a new PNMWriter suitable for
//               reading from this file type, if possible.  If writing
//               files of this type is not supported, returns NULL.
////////////////////////////////////////////////////////////////////
PNMWriter *PNMFileTypeIMG::
make_writer(FILE *file, bool owns_file) {
  init_pnm();
  return new Writer(this, file, owns_file);
}


inline unsigned long 
read_ulong(FILE *file) {
  unsigned long x;
  return pm_readbiglong(file, (long *)&x)==0 ? x : 0;
}

inline unsigned short 
read_ushort(FILE *file) {
  unsigned short x;
  return pm_readbigshort(file, (short *)&x)==0 ? x : 0;
}

inline unsigned char
read_uchar(FILE *file) {
  int x;
  x = getc(file);
  return (x!=EOF) ? (unsigned char)x : 0;
}

inline void
write_ulong(FILE *file, unsigned long x) {
  pm_writebiglong(file, (long)x);
}

inline void
write_ushort(FILE *file, unsigned long x) {
  pm_writebigshort(file, (short)(long)x);
}

inline void
write_uchar(FILE *file, unsigned char x) {
  putc(x, file);
}

////////////////////////////////////////////////////////////////////
//     Function: PNMFileTypeIMG::Reader::Constructor
//       Access: Public
//  Description: 
////////////////////////////////////////////////////////////////////
PNMFileTypeIMG::Reader::
Reader(PNMFileType *type, FILE *file, bool owns_file, string magic_number) : 
  PNMReader(type, file, owns_file)
{
  if (img_header_type == IHT_long) {
    if (!read_magic_number(_file, magic_number, 8)) {
      // Although raw IMG files have no magic number, they may have a
      // pair of ushorts or ulongs at the beginning to indicate the file
      // size.
      if (pnmimage_img_cat.is_debug()) {
	pnmimage_img_cat.debug()
	  << "IMG image file appears to be empty.\n";
      }
      _is_valid = false;
      return;
    }
    
    _x_size = 
      ((unsigned char)magic_number[0] << 24) |
      ((unsigned char)magic_number[1] << 16) |
      ((unsigned char)magic_number[2] << 8) |
      ((unsigned char)magic_number[3]);
    
    _y_size = 
      ((unsigned char)magic_number[4] << 24) |
      ((unsigned char)magic_number[5] << 16) |
      ((unsigned char)magic_number[6] << 8) |
      ((unsigned char)magic_number[7]);

  } else if (img_header_type == IHT_short) {
    if (!read_magic_number(_file, magic_number, 4)) {
      if (pnmimage_img_cat.is_debug()) {
	pnmimage_img_cat.debug()
	  << "IMG image file appears to be empty.\n";
      }
      _is_valid = false;
      return;
    }
    
    _x_size = 
      ((unsigned char)magic_number[0] << 8) |
      ((unsigned char)magic_number[1]);
    
    _y_size = 
      ((unsigned char)magic_number[2] << 8) |
      ((unsigned char)magic_number[3]);

  } else {
    _x_size = img_xsize;
    _y_size = img_ysize;
  }

  if (_x_size == 0 || _y_size == 0 ||
      _x_size > INSANE_SIZE || _y_size > INSANE_SIZE) {
    _is_valid = false;
    if (img_header_type == IHT_none) {
      pnmimage_img_cat.error()
	<< "Must specify img-xsize and img-ysize to load headerless raw files.\n";
    } else {
      pnmimage_img_cat.debug()
	<< "IMG file does not have a valid xsize,ysize header.\n";
    }
    return;
  }

  _maxval = 255;
  _num_channels = 3;

  if (pnmimage_img_cat.is_debug()) {
    pnmimage_img_cat.debug()
      << "Reading IMG " << *this << "\n";
  }
}

////////////////////////////////////////////////////////////////////
//     Function: PNMFileTypeIMG::Reader::supports_read_row
//       Access: Public, Virtual
//  Description: Returns true if this particular PNMReader supports a
//               streaming interface to reading the data: that is, it
//               is capable of returning the data one row at a time,
//               via repeated calls to read_row().  Returns false if
//               the only way to read from this file is all at once,
//               via read_data().
////////////////////////////////////////////////////////////////////
bool PNMFileTypeIMG::Reader::
supports_read_row() const {
  return true;
}

////////////////////////////////////////////////////////////////////
//     Function: PNMFileTypeIMG::Reader::read_row
//       Access: Public, Virtual
//  Description: If supports_read_row(), above, returns true, this
//               function may be called repeatedly to read the image,
//               one horizontal row at a time, beginning from the top.
//               Returns true if the row is successfully read, false
//               if there is an error or end of file.
////////////////////////////////////////////////////////////////////
bool PNMFileTypeIMG::Reader::
read_row(xel *row_data, xelval *) {
  int x;
  xelval red, grn, blu;
  for (x = 0; x < _x_size; x++) {
    red = read_uchar(_file);
    grn = read_uchar(_file);
    blu = read_uchar(_file);
    
    PPM_ASSIGN(row_data[x], red, grn, blu);
  }

  return true;
}

////////////////////////////////////////////////////////////////////
//     Function: PNMFileTypeIMG::Writer::Constructor
//       Access: Public
//  Description: 
////////////////////////////////////////////////////////////////////
PNMFileTypeIMG::Writer::
Writer(PNMFileType *type, FILE *file, bool owns_file) :
  PNMWriter(type, file, owns_file)
{
}

////////////////////////////////////////////////////////////////////
//     Function: PNMFileTypeIMG::Writer::supports_write_row
//       Access: Public, Virtual
//  Description: Returns true if this particular PNMWriter supports a
//               streaming interface to writing the data: that is, it
//               is capable of writing the image one row at a time,
//               via repeated calls to write_row().  Returns false if
//               the only way to write from this file is all at once,
//               via write_data().
////////////////////////////////////////////////////////////////////
bool PNMFileTypeIMG::Writer::
supports_write_row() const {
  return true;
}

////////////////////////////////////////////////////////////////////
//     Function: PNMFileTypeIMG::Writer::write_header
//       Access: Public, Virtual
//  Description: If supports_write_row(), above, returns true, this
//               function may be called to write out the image header
//               in preparation to writing out the image data one row
//               at a time.  Returns true if the header is
//               successfully written, false if there is an error.
//
//               It is the user's responsibility to fill in the header
//               data via calls to set_x_size(), set_num_channels(),
//               etc., or copy_header_from(), before calling
//               write_header().
////////////////////////////////////////////////////////////////////
bool PNMFileTypeIMG::Writer::
write_header() {
  if (img_header_type == IHT_long) {
    write_ulong(_file, _x_size);
    write_ulong(_file, _y_size);
  } else if (img_header_type == IHT_short) {
    write_ushort(_file, _x_size);
    write_ushort(_file, _y_size);
  }
  return true;
}

////////////////////////////////////////////////////////////////////
//     Function: PNMFileTypeIMG::Writer::write_row
//       Access: Public, Virtual
//  Description: If supports_write_row(), above, returns true, this
//               function may be called repeatedly to write the image,
//               one horizontal row at a time, beginning from the top.
//               Returns true if the row is successfully written,
//               false if there is an error.
//
//               You must first call write_header() before writing the
//               individual rows.  It is also important to delete the
//               PNMWriter class after successfully writing the last
//               row.  Failing to do this may result in some data not
//               getting flushed!
////////////////////////////////////////////////////////////////////
bool PNMFileTypeIMG::Writer::
write_row(xel *row_data, xelval *) {
  int x;
  for (x = 0; x < _x_size; x++) {
    write_uchar(_file, (unsigned char)(255*PPM_GETR(row_data[x])/_maxval));
    write_uchar(_file, (unsigned char)(255*PPM_GETG(row_data[x])/_maxval));
    write_uchar(_file, (unsigned char)(255*PPM_GETB(row_data[x])/_maxval));
  }
  
  return true;
}


