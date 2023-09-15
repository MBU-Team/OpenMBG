;-----------------------------------------------------------------------------
; Torque Game Engine
; Copyright (C) GarageGames.com, Inc.
;-----------------------------------------------------------------------------


%macro export_fn 1
   %ifdef LINUX
   ; No underscore needed for ELF object files
   global %1
   %1:
   %else
   global _%1
   _%1:
   %endif
%endmacro

segment .data

   struc quantVector
.numDim: resd 1
.pElem:  resd 1
.weight: resd 1
   endstruc

   struc sortVector
.pQuantVector: resd 1
.sortKey:      resd 1
   endstruc

fpcnst0              dd    0.0
fpcnst1              dd    1.0
fpcnstLudicrous      dq    1.0e200

currentWeight  dd    0
junkFp         dd    0

_numVectors    dd    0
_numDim        dd    0
_pVectors      dd    0

_ebp           dd    0
_numDimensions dd    0
lastVectorPtr  dd    0


segment .text



;      ARG pCenterOfMass:DWORD, \
;          pVectors:DWORD,      \
;          numVectors:DWORD
%define pCenterOfMass [ebp + 8]
%define pVectors      [ebp + 12]
%define numVectors    [ebp + 16]
export_fn asmGetStandardDev

    ; prologue
    push ebp
    mov ebp, esp

   ; Invariant register assignments:
   ;  ecx -    count remaining vectors
   ;  edx -    count remaining dimensions
   ;  esi -    point to elements of pCenterOfMass
   ;  edi -    points to current sortVector

   mov   esi, pCenterOfMass
   mov   esi, [esi + quantVector.pElem]
   
   mov   edi, pVectors
   
   mov   ecx, numVectors
   mov   [_numVectors], ecx
   
   mov   edx, [edi + sortVector.pQuantVector]
   mov   edx, [edx + quantVector.numDim]
   mov   [_numDim], edx

   ; Top most stack entry is standard deviation
   fld   dword [fpcnst0]

   ; Begin Vector Loop...
   mov   ecx, [_numVectors]
@@vectorLoopSD:
   ; Load zero to escape special case crap.
   fld   dword [fpcnst0]

   mov   eax, 0         ; eax will hold offset into array of doubles
                        ; ebx holds current vectors pElem
   mov   ebx, [edi + sortVector.pQuantVector]
   
   ; Store off the wieght of the vector
   fld   dword [ebx + quantVector.weight]
   fstp  dword [currentWeight]

   mov   ebx, [ebx + quantVector.pElem]
   mov   edx, [_numDim]
@@dimensionLoopSD:
   fld   dword [currentWeight]
   fld   dword [esi + eax]
   fld   dword [ebx + eax]
   ; st: pElem(currDim), COM(currDim), currentWeight, sumSoFar
   fsubp st1, st0
   fmul  st0, st0
   fmulp st1, st0
   faddp st1, st0
   ; st: sumSoFar

   add   eax, 4
   dec   edx
   jnz   @@dimensionLoopSD

   ; When we come out of this loop, the standard Dev for the previous vector
   ; is at the top of the stack.  Add it to the total deviation, which is the
   ; next entry up.
   faddp st1, st0

   ; step to the next vector
   add   edi, 8

   dec   ecx
   jnz   @@vectorLoopSD

   ; And we're done!  We return the standardDev in the first entry of
   ; the stack anyways, so we'll just leave it there...

   ; epilogue
   pop ebp
   ret



;      ARG pCenterOfMass:DWORD, \
;          pVectors:DWORD,      \
;          numVectors:DWORD,    \
;          totalWeight:DWORD
%define pCenterOfMass [ebp + 8]
%define pVectors [ebp + 12]
%define numVectors [ebp + 16]
%define totalWeight [ebp + 20]
export_fn asmCalcCenterOfMass
    ; prologue
    push ebp
    mov ebp, esp

   ; Invariant register assignments:
   ;  ecx -    count remaining vectors
   ;  edx -    count remaining dimensions
   ;  esi -    point to elements of pCenterOfMass
   ;  edi -    points to current sortVector

   mov   esi, pCenterOfMass
   mov   esi, [esi + quantVector.pElem]
   
   mov   edi, pVectors
   mov   [_pVectors], edi
   
   mov   ecx, numVectors
   mov   [_numVectors], ecx

   mov   eax, 0         ; eax will hold offset into array of doubles
                        ; ebx holds current vectors pElem
   
   mov   edx, [edi + sortVector.pQuantVector]
   mov   edx, [edx + quantVector.numDim]
   
   fld   dword [fpcnst1]
   fld   dword totalWeight
   fdivp st1, st0
@@dimensionLoopCM:
   ; Average for this dimension is first on stack.
   fld   dword [fpcnst0]

   mov   edi, [_pVectors]
   mov   ecx, [_numVectors]
@@vectorLoopCM:
   mov   ebx, [edi + sortVector.pQuantVector]
   fld   dword [ebx + quantVector.weight]
   mov   ebx, [ebx + quantVector.pElem]
   fld   dword [ebx + eax]
   fmulp st1, st0
   faddp st1, st0

   add   edi, 8
   dec   ecx
   jnz   @@vectorLoopCM
   
   fxch  st1
   fld   st0
   fxch  st2
   fmulp st1, st0

   fstp   dword [esi + eax]

   add   eax, 4
   dec   edx
   jnz   @@dimensionLoopCM

   ; Restore balanced fp stack...
   fstp  dword [junkFp]

   ; epilogue
   pop ebp
   ret


;      ARG pCenterOfMass:DWORD, \
;          pOutputMatrix:DWORD, \
;          numDimensions:DWORD, \
;          pVectors:DWORD,      \
;          numVectors:DWORD,    \
;          totalWeight:DWORD
%define pCenterOfMass [ebp+8]
%define pOutputMatrix [ebp+12]
%define numDimensions [ebp+16]
%define pVectors      [ebp+20]
%define numVectors    [ebp+24]
%define totalWeight   [ebp+30]
export_fn asmCalcCovarMatrix

    ; prologue
    push ebp
    mov ebp, esp

   mov   eax, 0                              ; EAX Tracks J index

   mov   esi, pCenterOfMass                  ; ESI points to COM

@@covarJLoopPt:
   cmp   eax, numDimensions
   jge   @@doneWithCovarMatrix

   mov   ebx, 0                              ; EBX Tracks I index

@@covarILoopPt:
   cmp   ebx, eax
   jg    @@doneWithRow
   
   ; To prevent any first time through nonsense
   fld   dword [fpcnst0]

   mov   ecx, numVectors
   mov   edx, pVectors
@@vectorLoop:
   mov   edi, [edx + sortVector.pQuantVector]
   add   edx, 8

   fld   dword [edi + quantVector.weight]
   mov   edi, [edi + quantVector.pElem]

                                             ; Implied top of stack is sumTotal
   fld   dword [edi + eax * 4]           ; ST = v_j, , weight
   fld   dword [esi + eax * 4]           ; ST = cm_j, v_j, weight
   fsubp st1, st0                        ; ST = (v_j - cm_j), weight

   fmulp st1, st0                        ; ST = ((v_j - cm_j) * weight)
   fld   dword [edi + ebx * 4]           ; ST = v_i, ((v_j - cm_j) * weight)
   fld   dword [esi + ebx * 4]           ; ST = cm_i, v_i, ((v_j - cm_j) * weight)
   fsubp st1, st0                        ; ST = (v_i - cm_i), ((v_j - cm_j) * weight)

   fmulp st1, st0                        ; ST = ((v_i * cm_i) * (v_j - cm_j) * weight))

   faddp st1, st0

   dec   ecx
   jnz   @@vectorLoop

   ; Write to appropriate output point...
   ;
   fld   dword totalWeight
   fdivp st1, st0

   mov   edi, pOutputMatrix
   mov   edi, [edi + ebx * 4]
   fstp  dword [edi + eax * 4]

   inc   ebx
   jmp   @@covarILoopPt

@@doneWithRow:

   inc   eax
   jmp   @@covarJLoopPt

@@doneWithCovarMatrix:

   ; epilogue
   pop ebp
   ret


;      ARG pTargetVector:DWORD,         \
;          pSourceVectors:DWORD,        \
;          numSourceVectors:DWORD,      \
;          numDimensions:DWORD
%define pTargetVector [ebp+8]
%define pSourceVectors [ebp+12]
%define numSourceVectors [ebp+16]
%define numDimensions [ebp+20]
export_fn asmFindClosestVectorNonEx

    ; prologue
    push ebp
    mov ebp, esp

   mov   eax, 12
   mov   ebx, numSourceVectors
   imul  ebx
   add   eax, pSourceVectors
   mov   [lastVectorPtr], eax

   mov   ebx, numDimensions
   mov   [_numDimensions], ebx

   mov   ebx, 0xffffffff                     ; EBX holds minSoFar
   fld   dword [fpcnstLudicrous]             ; Load ludicrous onto the stack

   mov   esi, pTargetVector
   mov   esi, [esi + quantVector.pElem]

   mov   [_ebp], ebp
   mov   ecx, pSourceVectors               ; ECX is current vector ptr
   mov   ebp, 0
@@sourceVectorLoop:
   mov   edi, [ecx + quantVector.pElem]
   fld   dword [fpcnst0]

   mov   edx, 0
@@dimLoop:
   fld   dword [esi + edx * 4]
   fld   dword [edi + edx * 4]
   fsubp st1, st0

   fld   st0
   fmulp st1, st0
   faddp st1, st0

   inc   edx
   cmp   edx, _numDimensions
   jl    @@dimLoop

   fcom   st1
   fnstsw ax
   test   ah,1
   je     @@notLessThan

   fxch  st1
   mov   ebx, ebp

@@notLessThan:
   fstp  dword [junkFp]

   inc   ebp
   add   ecx, 12
   cmp   ecx, lastVectorPtr
   jl    @@sourceVectorLoop

   ; Restore balanced fp stack...
   fstp  dword [junkFp]

   mov   eax, ebx
   mov   ebp, _ebp

   ; epilogue
   pop ebp
   ret
