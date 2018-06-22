#include "Deferred.h"

#include "MemoryDebug.h"

void DeferredInit(PyMOLGlobals * G, CDeferred * I)
{
  if(I) {
    I->G = G;
  }
}

void DeferredFree(CDeferred * I)
{
  while(I) {
    CDeferred *next = I->next;
    PyMolFreeP(I);
    I = next;
  }
}

CDeferred *DeferredExec(CDeferred * I)
{
  while(I) {                    /* executes all deferred actions */
    CDeferred *next = I->next;
    if(I->fn) {
      if(!I->fn(I)) {
        break;
      }
    }
    PyMolFreeP(I);
    I = next;
  }
  return I;
}
