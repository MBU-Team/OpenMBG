//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "platform/platform.h"
#include "core/dataChunker.h"


//----------------------------------------------------------------------------

DataChunker::DataChunker(S32 size)
{
   chunkSize          = size;
   curBlock           = NULL;
}

DataChunker::~DataChunker()
{
   freeBlocks();
}

void *DataChunker::alloc(S32 size)
{
    if (size > chunkSize)
    {
        DataBlock* temp = (DataBlock*)dMalloc(DataChunker::PaddDBSize + size);
        AssertFatal(temp, "Malloc failed");
        constructInPlace(temp);
        if (curBlock)
        {
            temp->next = curBlock->next;
            curBlock->next = temp;
        }
        else
        {
            curBlock = temp;
            temp->curIndex = chunkSize;
        }
        return temp->getData();
    }

    if (!curBlock || size + curBlock->curIndex > chunkSize)
    {
        const U32 paddDBSize = (sizeof(DataBlock) + 3) & ~3;
        DataBlock* temp = (DataBlock*)dMalloc(paddDBSize + chunkSize);
        AssertFatal(temp, "Malloc failed");
        constructInPlace(temp);
        temp->next = curBlock;
        curBlock = temp;
    }

    void* ret = curBlock->getData() + curBlock->curIndex;
    curBlock->curIndex += (size + 3) & ~3; // dword align
    return ret;
}

DataChunker::DataBlock::DataBlock()
{
    curIndex = 0;
    next = NULL;
}

DataChunker::DataBlock::~DataBlock()
{
    
}

void DataChunker::freeBlocks()
{
    while (curBlock && curBlock->next)
    {
        DataBlock* temp = curBlock->next;
        dFree(curBlock);
        curBlock = temp;
    }

    if (curBlock)
    {
        curBlock->curIndex = 0;
        curBlock->next = NULL;
    }
}

