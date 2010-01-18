/*
  File: typepatch.c
  Author: James Oakley
  Project: Katana (preliminary work)
  Date: January 10
  Description: Preliminary patching program just for patching main_v0.c's structure Foo to have an extra field
               The first version of this program is going to be very simple. It will perform the following steps
               1. Load main_v0 with dwarfdump and determine where the variable bar is stored
               2. Find all references to the variable bar
               3. Attach to the running process with ptrace
               4. Make the process execute mmap to allocate space for a new data segment
               5. Copy over field1 and field2 from Foo bar into the new memory area and zero the added field
               6. Fixup all locations referring to the old address of bar with the new address and set the offset accordingly (should be able to get information for fixups from the rel.text section)
  Usage: typepatch PID
         PID is expected to be a running process build from main_v0.c
*/
#include <errno.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <signal.h>
#include <sys/wait.h>
#include <libdwarf.h>
#include "dwarf.h"
#include "dwarftypes.h"
#include "util.h"
#include "types.h"
#include "target.h"
#include "elfparse.h"
#include "hotpatch.h"
int pid;//pid of running process

//test relocation of the variable bar
//knowing some very specific things about it
void testManualRelocateBar()
{
  int barSymIdx=getSymtabIdx("bar");
  int barAddress=getSymAddress(barSymIdx);
  printf("bar located at %x\n",(unsigned int)barAddress);

  List* relocItems=getRelocationItemsFor(barSymIdx);
  //allocate a new page for us to put the new variable in
  long newPageAddr=mmapTarget(sysconf(_SC_PAGE_SIZE),PROT_READ|PROT_WRITE);
  printf("mapped in a new page at 0x%x\n",(uint)newPageAddr);

  //now set the data in that page
  //first get it from the old address
  uint barData[4];
  memcpyFromTarget((char*)barData,barAddress,sizeof(int)*3);
  printf("read data %i,%i,%i\n",barData[0],barData[1],barData[2]);
  //copy it to the new address
  memcpyToTarget(newPageAddr,(char*)barData,sizeof(int)*3);

  //now test to make sure we copied it correctly
  memcpyFromTarget((char*)barData,newPageAddr,sizeof(int)*3);
  printf("read new data %i,%i,%i\n",barData[0],barData[1],barData[2]);
  
  for(List* li=relocItems;li;li=li->next)
  {
    GElf_Rel* rel=li->value;
    printf("relocation for bar at %x with type %i\n",(unsigned int)rel->r_offset,(unsigned int)ELF64_R_TYPE(rel->r_info));
    addr_t oldAddr=getTextAtRelOffset(rel->r_offset);
    printf("old addr is 0x%x\n",(uint)oldAddr);
    uint newAddr=newPageAddr+(oldAddr-barAddress);
    //*oldAddr=newAddr;
    modifyTarget(rel->r_offset,newAddr);
  }
}

//again test relocation of the variable bar using specific
//knowledge we have about the source program, but this
//time use the fixupVariable function and the type transformation
//structures that will be automatically created in the future.
void testManualRelocateAndTransformBar()
{
  TypeInfo fooType;
  fooType.name=strdup("Foo");
  fooType.length=12;
  fooType.numFields=3;
  fooType.fields=zmalloc(sizeof(char*)*3);
  fooType.fieldLengths=zmalloc(sizeof(int)*3);
  fooType.fieldTypes=zmalloc(sizeof(char*)*3);
  fooType.fields[0]="field1";
  fooType.fields[1]="field2";
  fooType.fields[2]="field3";
  fooType.fieldLengths[0]=fooType.fieldLengths[1]=fooType.fieldLengths[2]=sizeof(int);
  fooType.fieldTypes[0]=strdup("int");
  fooType.fieldTypes[1]=strdup("int");
  fooType.fieldTypes[2]=strdup("int");
  TypeInfo fooType2;//version with an extra field
  fooType2.name=strdup("Foo");
  fooType2.length=16;
  fooType2.numFields=3;
  fooType2.fields=zmalloc(sizeof(char*)*4);
  fooType2.fieldLengths=zmalloc(sizeof(int)*4);
  fooType2.fieldTypes=zmalloc(sizeof(char*)*4);
  fooType2.fields[0]="field1";
  fooType2.fields[1]="field_extra";
  fooType2.fields[2]="field2";
  fooType2.fields[3]="field3";
  fooType2.fieldLengths[0]=fooType2.fieldLengths[1]=fooType2.fieldLengths[2]=fooType2.fieldLengths[3]=sizeof(int);
  fooType2.fieldTypes[0]=strdup("int");
  fooType2.fieldTypes[1]=strdup("int");
  fooType2.fieldTypes[2]=strdup("int");
  fooType2.fieldTypes[3]=strdup("int");
  VarInfo barInfo;
  barInfo.name="bar";
  barInfo.type=&fooType;
  TypeTransform trans;
  trans.from=&fooType;
  trans.to=&fooType2;
  trans.fieldOffsets=zmalloc(sizeof(int)*3);
  trans.fieldOffsets[0]=0;
  trans.fieldOffsets[1]=8;
  trans.fieldOffsets[2]=12;
  TransformationInfo transInfo;
  transInfo.typeTransformers=dictCreate(10);//todo: 10 a magic number
  dictInsert(transInfo.typeTransformers,barInfo.type->name,&trans);
  transInfo.varsToTransform=NULL;//don't actually need it for fixupVariable
  transInfo.freeSpaceLeft=0;
  fixupVariable(barInfo,&transInfo,pid);
  //todo: free memory
}

void diffAndFixTypes(DwarfInfo* diPatchee,DwarfInfo* diPatched)
{
  //todo: this assumes both have the same number of compilation units
  //and that their order corresponds. This is not a good assumption to make
  List* cuLi1=diPatchee->compilationUnits;
  List* cuLi2=diPatched->compilationUnits;
  for(;cuLi1 && cuLi2;cuLi1=cuLi1->next,cuLi2=cuLi2->next)
  {
    TransformationInfo* trans=zmalloc(sizeof(TransformationInfo));
    trans->typeTransformers=dictCreate(100);//todo: get rid of magic # 100
    CompilationUnit* cu1=cuLi1->value;
    CompilationUnit* cu2=cuLi2->value;
    printf("Examining compilation unit %s\n",cu1->name);
    VarInfo** vars1=(VarInfo**)dictValues(cu1->tv->globalVars);
    //todo: handle addition of variables in the patch
    VarInfo* var=vars1[0];
    Dictionary* patchVars=cu2->tv->globalVars;
    for(int i=0;var;i++,var=vars1[i])
    {
      printf("Found variable %s \n",var->name);
      VarInfo* patchedVar=dictGet(patchVars,var->name);
      if(!patchedVar)
      {
        //todo: do we need to do anything special to handle removal of variables in the patch?
        printf("warning: var %s seems to have been removed in the patch\n",var->name);
        continue;
      }
      TypeInfo* ti1=var->type;
      TypeInfo* ti2=patchedVar->type;
      bool needsTransform=false;
      if(dictExists(trans->typeTransformers,ti1->name))
      {
        needsTransform=true;
      }
      else
      {
        //todo: should have some sort of caching for types we've already
        //determined to be equal
        TypeTransform* transform=NULL;
        if(!compareTypes(ti1,ti2,&transform))
        {
          if(!transform)
          {
            //todo: may not want to actually abort, may just want to issue
            //an error
            fprintf(stderr,"Error, cannot generate type transformation for variable %s\n",var->name);
            abort();
          }
          printf("generated type transformation for type %s\n",ti1->name);
          dictInsert(trans->typeTransformers,ti1->name,transform);
          needsTransform=true;
        }
      }
      if(needsTransform)
      {
        List* li=zmalloc(sizeof(List));
        li->value=var;
        if(trans->varsToTransform)
        {
          trans->varsToTransformEnd->next=li;
        }
        else
        {
          trans->varsToTransform=li;
        }
        trans->varsToTransformEnd=li;
      }
    }
    //now actually transform the variables
    List* li=trans->varsToTransform;
    for(;li;li=li->next)
    {
      VarInfo* var=(VarInfo*)li->value;
      printf("fixing variable %s\n",var->name);
      fixupVariable(*var,trans,pid);
    }
    printf("completed all transformations for compilation unit %s\n",cu1->name);
    freeTransformationInfo(trans);
    free(vars1);
    
  }
}

int main(int argc,char** argv)
{
  if(argc<2)
  {
    fprintf(stderr,"must specify pid to attach to\n");
    exit(1);
  }
  pid=atoi(argv[1]);
  if(elf_version(EV_CURRENT)==EV_NONE)
  {
    fprintf(stderr,"Failed to init ELF library\n");
    exit(1);
  }
  Elf* e=openELFFile("patched");
  DwarfInfo* diPatched=readDWARFTypes(e);
  endELF(e);
  e=openELFFile("patchee");
  findELFSections();
  DwarfInfo* diPatchee=readDWARFTypes(e);

  startPtrace();
  diffAndFixTypes(diPatchee,diPatched);
  


  //printSymTab();
  

  //testManualRelocateBar();
  //testManualRelocateAndTransformBar();
  endPtrace();

  //flush modifications to disk
  //except not now, now trying to modify it in memory
  //writeOut("newpatchee");
  //all done
  endELF(e);
  return 0;
}

