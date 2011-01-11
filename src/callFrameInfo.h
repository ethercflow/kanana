/*
  File: callFrameInfo.h
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

  Project: Katana
  Date: January 2011
  Description: Data structures and methods dealing with call frame information
*/


#ifndef callFrameInfo_h
#define callFrameInfo_h
#include "dwarf_instr.h"
#include <libelf.h>


typedef struct CallFrameInfo
{
  struct FDE* fdes;//for relocatable and executable objects, these
                   //will be sorted by lowpc
  int numFDEs;
  struct CIE* cies;
  int numCIEs;
  bool isEHFrame;
  //needed when encoding eh_frame because of pc-relative pointer encodings
  //todo: should really support relocations and then don't need to know this now
  addr_t sectionAddress;
  addr_t ehHdrAddress;//address of .eh_frame_hdr
  //exception handling table, info that would be stored in
  //.gcc_except_frame
  struct ExceptTable* exceptTable;
} CallFrameInfo;

typedef enum
{
  CAF_DATA_PRESENT=1,//set if augmentation data is present
  CAF_FDE_ENC=2,//set if the fdePointerEncoding member of the
                        //augmentationInfo struct is valid
  CAF_FDE_LSDA=4,//fdeLSDAPointerEncoding is valid. The FDE will have an LSDA pointer
} CIEAugmentFlags;

typedef struct CIE
{
  RegInstruction* initialInstructions;
  int numInitialInstructions;
  Dictionary* initialRules; //dictionary mapping the stringified
                            //version of a register to the rule (of
                            //type PoRegRule) for setting that
                            //register
  //todo: I don't think this memory is currently freed
  char* augmentation;//augmentation string. Not needed for patching
                     //but needed for some more general-purpose DWARF
                     //manipulations
  Dwarf_Signed dataAlign;
  Dwarf_Unsigned codeAlign;
  Dwarf_Half returnAddrRuleNum;
  int idx;//what index cie this is in a DWARF section
  Dwarf_Small version;
  int addressSize;//the size of a target address for this CIE and FDEs that use it
  int segmentSize;//unused for most systems. Included for
                  //compatibility with the DWARF specification. Size
                  //of a segment selector.
  Dwarf_Unsigned augmentationDataLen;
  byte* augmentationData;
  //see the Linux Standards Base at
  //http://refspecs.freestandards.org/LSB_4.0.0/LSB-Core-generic/LSB-Core-generic/ehframechpt.html
  //for augmentation data information
  struct
  {
    byte flags;//OR'd CIEAugmentFlags
    byte fdePointerEncoding;
    byte fdeLSDAPointerEncoding;
  } augmentationInfo;
} CIE;


typedef struct FDE
{
  CIE* cie;
  RegInstruction* instructions;
  int numInstructions;
  int memSize;//size of memory area the FDE describes. Used when
              //fixing up pointers to know how much mem to
              //allocate. Has no meaning if this FDE wasn't read from
              //a PO
  int lowpc;
  int highpc;//has no meaning if this FDE describes fixups and was
             //read from a PO
  int offset;//offset from beginning of debug frame. Note that this
             //field is fairly subject to change. For an FDE built
             //from dwarfscript it may not be set at all. For an FDE
             //loaded from a binary it will be set. 
  int idx;//what index fde this is in a DWARF section
  Dwarf_Unsigned augmentationDataLen;
  byte* augmentationData;
  struct
  {
    //if this is false than all the other fields in this struct are invalid
    bool filled;
    addr_t LSDAPointer;
  } augmentationInfo;
} FDE;

//struct for raw data returned from buildCallFrameSectionData
typedef struct
{
  byte* ehData;
  int ehDataLen;
  byte* ehHdrData;
  int ehHdrDataLen;
} CallFrameSectionData;

//the info contained in a call-site table entry in a LSDA in
//.gcc_except_frame
typedef struct
{
  addr_t position;
  word_t length;
  addr_t landingPadPosition;
  idx_t firstAction;//index into the action table. Only valid if hasAction is true
  bool hasAction;
} CallSiteRecord;

//entry in the action table in an LSDA in .gcc_except_frame
typedef struct
{
  idx_t typeFilterIndex;//index into the typeTable associated with the LSDA
  idx_t nextAction;//the index into the actionTable associated with
                   //the LSDA of the next action record in the chain
                   //of action records to look at.
} ActionRecord;


typedef struct
{
  addr_t lpStart;
  CallSiteRecord* callSiteTable;
  int numCallSites;//number of CallSiteEntrys in callSiteTable
  ActionRecord* actionTable;
  int numActionEntries;
  //todo: will the type table entries ever be longer than a word? They
  //don't seem to be in what gcc emits, and this is after all gcc
  //specific stuff (although LLVM follows it as well)
  word_t* typeTable;
  int numTypeEntries;
} LSDA;

//info that would be stored in .gcc_except_table
typedef struct ExceptTable
{
  LSDA* lsdas;
  int numLSDAs;
} ExceptTable;

//returns a void* to a binary representation of a Dwarf call frame
//information section (i.e. .debug_frame in the dwarf specification)
//the length of the returned buffer is written into byteLen.
//the memory for the buffer should free'd when the caller is finished with it
CallFrameSectionData buildCallFrameSectionData(CallFrameInfo* cfi);

//reads the CIE's augmentationData and tries to set up
//the cie->augmentationInfo
//see the Linux Standards Base at
//http://refspecs.freestandards.org/LSB_4.0.0/LSB-Core-generic/LSB-Core-generic/ehframechpt.html
//for augmentation data information
void parseAugmentationStringAndData(CIE* cie);
void parseFDEAugmentationData(FDE* fde,addr_t augDataAddress);

void printEHPointerEncoding(FILE* file,byte encoding);

//builds an ExceptTable object from the raw ELF section
ExceptTable parseExceptFrame(Elf_Scn* scn);

#endif
