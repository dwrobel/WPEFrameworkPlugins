
#include "Module.h"
#include "OCDM.h"
#include <interfaces/json/JsonData_OpenCDMi.h>

namespace WPEFramework {

namespace Plugin {

    using namespace JsonData::OCDM;

    // Registration
    //

    void OCDM::RegisterAll()
    {
        Property<Core::JSON::ArrayType<DrmData>>(_T("drms"), &OCDM::get_drms, nullptr, this);
        Property<Core::JSON::ArrayType<Core::JSON::String>>(_T("keysystems"), &OCDM::get_keysystems, nullptr, this);
    }

    void OCDM::UnregisterAll()
    {
        Unregister(_T("keysystems"));
        Unregister(_T("drms"));
    }

    bool OCDM::KeySystems(const string& name, Core::JSON::ArrayType<Core::JSON::String>& response) const
    {
        bool result = false;
        RPC::IStringIterator* keySystemsIter(_opencdmi->Designators(name));
        if (keySystemsIter != nullptr) {
            string element;
            while (keySystemsIter->Next(element) == true) {
                response.Add(Core::JSON::String(element));
            }

            keySystemsIter->Release();
            result = true;
        }

        return result;
    }

    // API implementation
    //

    // Property: drms - Supported DRM systems
    // Return codes:
    //  - ERROR_NONE: Success
    uint32_t OCDM::get_drms(Core::JSON::ArrayType<DrmData>& response) const
    {
        RPC::IStringIterator* drmsIter(_opencdmi->Systems());
        if (drmsIter != nullptr) {
            string element;
            while (drmsIter->Next(element) == true) {
                DrmData drm;
                drm.Name = element;
                KeySystems(element, drm.Keysystems);
                response.Add(drm);
            }

            drmsIter->Release();
        }

        return Core::ERROR_NONE;
    }

    // Property: keysystems - DRM key systems
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_BAD_REQUEST: Invalid DRM name
    uint32_t OCDM::get_keysystems(const string& index, Core::JSON::ArrayType<Core::JSON::String>& response) const
    {
        uint32_t result = Core::ERROR_BAD_REQUEST;

        if (KeySystems(index, response)) {
            result = Core::ERROR_NONE;
        }

        return result;
    }

} // namespace Plugin

}

