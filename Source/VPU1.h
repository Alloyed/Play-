#ifndef _VPU1_H_
#define _VPU1_H_

#include "VPU.h"

class CVPU1 : public CVPU
{
public:
                    CVPU1(CVIF&, unsigned int, const CVIF::VPUINIT&);
    virtual void    SaveState(CZipArchiveWriter&);
    virtual void    LoadState(CZipArchiveReader&);
    virtual uint32  GetTOP() const;
    virtual void    Reset();

protected:
    virtual void    ExecuteCommand(StreamType&, CODE);
    virtual void    Cmd_DIRECT(StreamType&, CODE);
    virtual void    Cmd_UNPACK(StreamType&, CODE, uint32);

private:
    virtual void    StartMicroProgram(uint32);

    uint32          m_BASE;
    uint32          m_OFST;
    uint32          m_TOP;
    uint32          m_TOPS;
};

#endif