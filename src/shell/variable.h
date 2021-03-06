/*
  File: shell/variable.h
  Author: James Oakley
  Copyright (C): 2010 Dartmouth College
  License: Katana is free software: you may redistribute it and/or
  modify it under the terms of the GNU General Public License as
  published by the Free Software Foundation, either version 2 of the
  License, or (at your option) any later version. Regardless of
  which version is chose, the following stipulation also applies:
    
  Any redistribution must include copyright notice attribution to
  Dartmouth College as well as the Warranty Disclaimer below, as well as
  this list of conditions in any related documentation and, if feasible,
  on the redistributed software; Any redistribution must include the
  acknowledgment, “This product includes software developed by Dartmouth
  College,” in any related documentation and, if feasible, in the
  redistributed software; and The names “Dartmouth” and “Dartmouth
  College” may not be used to endorse or promote products derived from
  this software.  

  WARRANTY DISCLAIMER

  PLEASE BE ADVISED THAT THERE IS NO WARRANTY PROVIDED WITH THIS
  SOFTWARE, TO THE EXTENT PERMITTED BY APPLICABLE LAW. EXCEPT WHEN
  OTHERWISE STATED IN WRITING, DARTMOUTH COLLEGE, ANY OTHER COPYRIGHT
  HOLDERS, AND/OR OTHER PARTIES PROVIDING OR DISTRIBUTING THE SOFTWARE,
  DO SO ON AN "AS IS" BASIS, WITHOUT WARRANTY OF ANY KIND, EITHER
  EXPRESSED OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
  PURPOSE. THE ENTIRE RISK AS TO THE QUALITY AND PERFORMANCE OF THE
  SOFTWARE FALLS UPON THE USER OF THE SOFTWARE. SHOULD THE SOFTWARE
  PROVE DEFECTIVE, YOU (AS THE USER OR REDISTRIBUTOR) ASSUME ALL COSTS
  OF ALL NECESSARY SERVICING, REPAIR OR CORRECTIONS.

  IN NO EVENT UNLESS REQUIRED BY APPLICABLE LAW OR AGREED TO IN WRITING
  WILL DARTMOUTH COLLEGE OR ANY OTHER COPYRIGHT HOLDER, OR ANY OTHER
  PARTY WHO MAY MODIFY AND/OR REDISTRIBUTE THE SOFTWARE AS PERMITTED
  ABOVE, BE LIABLE TO YOU FOR DAMAGES, INCLUDING ANY GENERAL, SPECIAL,
  INCIDENTAL OR CONSEQUENTIAL DAMAGES ARISING OUT OF THE USE OR
  INABILITY TO USE THE SOFTWARE (INCLUDING BUT NOT LIMITED TO LOSS OF
  DATA OR DATA BEING RENDERED INACCURATE OR LOSSES SUSTAINED BY YOU OR
  THIRD PARTIES OR A FAILURE OF THE PROGRAM TO OPERATE WITH ANY OTHER
  PROGRAMS), EVEN IF SUCH HOLDER OR OTHER PARTY HAS BEEN ADVISED OF THE
  POSSIBILITY OF SUCH DAMAGES.
    
  The complete text of the license may be found in the file COPYING
  which should have been distributed with this software. The GNU
  General Public License may be obtained at
  http://www.gnu.org/licenses/gpl.html
    
  Project:  Katana
  Date: December 2010
  Description: Class representing variables used in the Katana shell
*/


#ifndef variable_h
#define variable_h

#include "param.h"
extern "C"
{
#include "elfparse.h"
}




//this is a helper class used for storing the actual data associated
//with the variable. It is subclassed for the different data types
//most of the actual functionality is handed off to it. The reason
//that variable itself is not subclassed is that it would then be
//difficult to change the type of a variable and we do not want to enforce static
//typing
class ShellVariableData
{
public:
  ShellVariableData();
  virtual ~ShellVariableData(){}
  //the returned pointer is valid until the next call to getData
  virtual ParamDataResult* getData(ShellParamCapability dataType,int idx=0);
  //some types of variables can hold multiple pieces of the same sort
  //of data, for example dwarfscript compile can emit a variable which
  //may contain data for multiple sections
  virtual int getEntityCount();
  virtual bool isCapable(ShellParamCapability cap,int idx=0);
protected:
  void initResult();
  ParamDataResult* result;
};


class ShellVariable : public ShellParam
{
 public:
  ShellVariable(char* name);
  virtual ~ShellVariable();
  void setValue(ElfInfo* e);
  //note that the data is not copied and will be freed when it is no
  //longer needed. Copy the data first if you need to retain control
  //over it.
  void setValue(byte* data,int dataLen);
  void setValue(byte* data,int dataLen,SectionHeaderData* header);
  virtual void setValue(char* string);
  void makeArray(ShellVariableData** items,int cnt);

  //the returned pointer is valid until the next call to getData
  virtual ParamDataResult* getData(ShellParamCapability dataType,int idx=0);
  //some types of variables can hold multiple pieces of the same sort
  //of data, for example dwarfscript compile can emit a variable which
  //may contain data for multiple sections
  virtual int getEntityCount();
  //idx indicates whether the given data is supported for the given index
  virtual bool isCapable(ShellParamCapability cap,int idx=0);
  //for use from within C functions
  static void deleteShellVariable(void* var)
  {
    delete (ShellVariable*)var;
  }
 protected:
  char* name;
  ShellVariableData* data;
  
};



#endif
// Local Variables:
// mode: c++
// End:
