#include "VuExecutor.h"

CVuExecutor::CVuExecutor(CMIPS& context) :
CMipsExecutor(context)
{

}

CVuExecutor::~CVuExecutor()
{

}

void CVuExecutor::PartitionFunction(uint32 functionAddress)
{
    const uint32 vuMaxAddress = 0x4000;
    typedef std::set<uint32> PartitionPointSet;
    uint32 endAddress = 0;
    PartitionPointSet partitionPoints;

    //Insert begin point
    partitionPoints.insert(functionAddress);

    //Find the end
    for(uint32 address = functionAddress; ; address += 4)
    {
        //Probably going too far...
        if(address >= vuMaxAddress)
        {
            endAddress = address;
            partitionPoints.insert(endAddress);
            break;
        }

        uint32 opcode = m_context.m_pMemoryMap->GetInstruction(address);
        //If we find the E bit in an upper instruction
        if((address & 0x04) && (opcode & 0x40000000))
        {
            endAddress = address + 8;
            partitionPoints.insert(endAddress + 4);
            break;
        }
    }

    //Find partition points within the function
    for(uint32 address = functionAddress; address <= endAddress; address += 4)
    {
        uint32 opcode = m_context.m_pMemoryMap->GetInstruction(address);
        bool isBranch = m_context.m_pArch->IsInstructionBranch(&m_context, address, opcode);
        if(isBranch)
        {
            partitionPoints.insert(address + 0x10);
            uint32 target = m_context.m_pArch->GetInstructionEffectiveAddress(&m_context, address, opcode);
            if(target > functionAddress && target < endAddress)
            {
                partitionPoints.insert(target);
            }
        }
        //-- Meaningless in VU
        //SYSCALL or ERET
        //if(opcode == 0x0000000C || opcode == 0x42000018)
        //{
        //    partitionPoints.insert(address + 4);
        //}
        //Check if there's a block already exising that this address
        if(address != endAddress)
        {
            CBasicBlock* possibleBlock = FindBlockStartingAt(address);
            if(possibleBlock != NULL)
            {
                assert(possibleBlock->GetEndAddress() <= endAddress);
                //Add its beginning and end in the partition points
                partitionPoints.insert(possibleBlock->GetBeginAddress());
                partitionPoints.insert(possibleBlock->GetEndAddress() + 4);
            }
        }
    }

    uint32 currentPoint = MIPS_INVALID_PC;
    for(PartitionPointSet::const_iterator pointIterator(partitionPoints.begin());
        pointIterator != partitionPoints.end(); pointIterator++)
    {
        if(currentPoint != MIPS_INVALID_PC)
        {
            uint32 beginAddress = currentPoint;
            uint32 endAddress = *pointIterator - 4;
            //Sanity checks
            assert((beginAddress & 0x07) == 0x00);
            assert((endAddress & 0x07) == 0x04);
            CreateBlock(beginAddress, endAddress);
        }
        currentPoint = *pointIterator;
    }
}