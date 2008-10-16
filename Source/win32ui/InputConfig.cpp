#include "InputConfig.h"
#include "win32/GuidUtils.h"
#include <string.h>
#include <boost/lexical_cast.hpp>

using namespace std;
using namespace Framework;
using namespace boost;
using namespace PS2;

#define CONFIG_PREFIX               ("input")
#define CONFIG_BINDING_TYPE         ("bindingtype")

#define CONFIG_SIMPLEBINDING_PREFIX ("simplebinding")

#define CONFIG_BINDINGINFO_DEVICE   ("device")
#define CONFIG_BINDINGINFO_ID       ("id")

#define CONFIG_SIMULATEDAXISBINDING_PREFIX  ("simulatedaxisbinding")
#define CONFIG_SIMULATEDAXISBINDING_KEY1    ("key1")
#define CONFIG_SIMULATEDAXISBINDING_KEY2    ("key2")

CInputConfig::CInputConfig(CAppConfig& config)
{
    for(unsigned int i = 0; i < CControllerInfo::MAX_BUTTONS; i++)
    {
        string prefBase = CConfig::MakePreferenceName(CONFIG_PREFIX, CControllerInfo::m_buttonName[i]);
        config.RegisterPreferenceInteger(
            CConfig::MakePreferenceName(prefBase, CONFIG_BINDING_TYPE).c_str(),
            0);
        CSimpleBinding::RegisterPreferences(config, prefBase.c_str());
    }
    Load();
}

CInputConfig::~CInputConfig()
{

}

void CInputConfig::Load()
{
    for(unsigned int i = 0; i < CControllerInfo::MAX_BUTTONS; i++)
    {
        BINDINGTYPE bindingType = BINDING_UNBOUND;
        string prefBase = CConfig::MakePreferenceName(CONFIG_PREFIX, CControllerInfo::m_buttonName[i]);
        bindingType = static_cast<BINDINGTYPE>(CAppConfig::GetInstance().GetPreferenceInteger((prefBase + "." + string(CONFIG_BINDING_TYPE)).c_str()));
        if(bindingType == BINDING_UNBOUND) continue;
        BindingPtr binding;
        switch(bindingType)
        {
        case BINDING_SIMPLE:
            binding.reset(new CSimpleBinding());
            break;
        }
        if(binding)
        {
            binding->Load(CAppConfig::GetInstance(), prefBase.c_str());
        }
        m_bindings[i] = binding;
    }
}

void CInputConfig::Save()
{
    for(unsigned int i = 0; i < CControllerInfo::MAX_BUTTONS; i++)
    {
        BindingPtr& binding = m_bindings[i];
        if(binding == NULL) continue;
        string prefBase = CConfig::MakePreferenceName(CONFIG_PREFIX, CControllerInfo::m_buttonName[i]);
        CAppConfig::GetInstance().SetPreferenceInteger(
            CConfig::MakePreferenceName(prefBase, CONFIG_BINDING_TYPE).c_str(), 
            binding->GetBindingType());
        binding->Save(CAppConfig::GetInstance(), prefBase.c_str());
    }
}

const CInputConfig::CBinding* CInputConfig::GetBinding(CControllerInfo::BUTTON button) const
{
    if(button >= CControllerInfo::MAX_BUTTONS)
    {
        throw exception();
    }
    return m_bindings[button].get();
}

void CInputConfig::SetSimpleBinding(CControllerInfo::BUTTON button, const BINDINGINFO& binding)
{
    if(button >= CControllerInfo::MAX_BUTTONS)
    {
        throw exception();
    }
    m_bindings[button].reset(new CSimpleBinding(binding.device, binding.id));
}

void CInputConfig::TranslateInputEvent(const GUID& device, uint32 id, uint32 value, const InputEventHandler& eventHandler)
{
    for(unsigned int i = 0; i < CControllerInfo::MAX_BUTTONS; i++)
    {
        BindingPtr& binding = m_bindings[i];
        if(!binding) continue;
        binding->ProcessEvent(device, id, value, static_cast<CControllerInfo::BUTTON>(i), eventHandler);
    }
}

tstring CInputConfig::GetBindingDescription(DirectInput::CManager* directInputManager, CControllerInfo::BUTTON button) const
{
    assert(button < CControllerInfo::MAX_BUTTONS);
    const BindingPtr& binding = m_bindings[button];
    if(binding)
    {
        return binding->GetDescription(directInputManager);
    }
    else
    {
        return _T("");
    }
}

////////////////////////////////////////////////
// SimpleBinding
////////////////////////////////////////////////

CInputConfig::CSimpleBinding::CSimpleBinding(const GUID& device, uint32 id) :
BINDINGINFO(device, id)
{

}

CInputConfig::CSimpleBinding::~CSimpleBinding()
{

}

CInputConfig::BINDINGTYPE CInputConfig::CSimpleBinding::GetBindingType() const
{
    return BINDING_SIMPLE;
}

void CInputConfig::CSimpleBinding::Save(CConfig& config, const char* buttonBase) const
{
    string prefBase = CConfig::MakePreferenceName(buttonBase, CONFIG_SIMPLEBINDING_PREFIX);
    config.SetPreferenceString(
        CConfig::MakePreferenceName(prefBase, CONFIG_BINDINGINFO_DEVICE).c_str(), 
        lexical_cast<string>(device).c_str());
    config.SetPreferenceInteger(
        CConfig::MakePreferenceName(prefBase, CONFIG_BINDINGINFO_ID).c_str(), 
        id);
}

void CInputConfig::CSimpleBinding::Load(CConfig& config, const char* buttonBase)
{
    string prefBase = CConfig::MakePreferenceName(buttonBase, CONFIG_SIMPLEBINDING_PREFIX);
    device = lexical_cast<GUID>(config.GetPreferenceString(CConfig::MakePreferenceName(prefBase, CONFIG_BINDINGINFO_DEVICE).c_str()));
    id = config.GetPreferenceInteger(CConfig::MakePreferenceName(prefBase, CONFIG_BINDINGINFO_ID).c_str());
}

tstring CInputConfig::CSimpleBinding::GetDescription(DirectInput::CManager* directInputManager) const
{
    DIDEVICEINSTANCE deviceInstance;
    DIDEVICEOBJECTINSTANCE objectInstance;
    if(!directInputManager->GetDeviceInfo(device, &deviceInstance))
    {
        return _T("");
    }
    if(!directInputManager->GetDeviceObjectInfo(device, id, &objectInstance))
    {
        return _T("");
    }
    return tstring(deviceInstance.tszInstanceName) + _T(": ") + tstring(objectInstance.tszName);
}

void CInputConfig::CSimpleBinding::ProcessEvent(const GUID& device, uint32 id, uint32 value, PS2::CControllerInfo::BUTTON button, const InputEventHandler& eventHandler)
{
    if(id != BINDINGINFO::id) return;
    if(device != BINDINGINFO::device) return;
    eventHandler(button, value);
}

void CInputConfig::CSimpleBinding::RegisterPreferences(CConfig& config, const char* buttonBase)
{
    string prefBase = CConfig::MakePreferenceName(buttonBase, CONFIG_SIMPLEBINDING_PREFIX);
    config.RegisterPreferenceString(
        CConfig::MakePreferenceName(prefBase, CONFIG_BINDINGINFO_DEVICE).c_str(), 
        lexical_cast<string>(GUID()).c_str());
    config.RegisterPreferenceInteger(
        CConfig::MakePreferenceName(prefBase, CONFIG_BINDINGINFO_ID).c_str(), 
        0);
}

////////////////////////////////////////////////
// SimulatedAxisBinding
////////////////////////////////////////////////

void CInputConfig::CSimulatedAxisBinding::RegisterPreferences(CConfig& config, const char* buttonBase)
{
//    string prefBase = string(buttonBase) + "." + string(CONFIG_SIMPLEBINDING_PREFIX);
//    config.RegisterPreferenceString(
//        (prefBase + "." + string(CONFIG_SIMPLEBINDING_DEVICE)).c_str(), lexical_cast<string>(GUID()).c_str());
//    config.RegisterPreferenceInteger(
//        (prefBase + "." + string(CONFIG_SIMPLEBINDING_ID)).c_str(), 0);
}