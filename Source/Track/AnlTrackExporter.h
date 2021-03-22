#pragma once

#include "AnlTrackModel.h"

ANALYSE_FILE_BEGIN

namespace Track
{
    class Exporter
    {
    public:
        
        static void toPreset(Accessor const& accessor, AlertType const alertType);
        
        static void fromPreset(Accessor& accessor, AlertType const alertType);
        
        static void toTemplate(Accessor const& accessor, AlertType const alertType);
        
        static void toImage(Accessor const& accessor, AlertType const alertType);
        
        static void toCsv(Accessor const& accessor, AlertType const alertType);
        
        static void toXml(Accessor const& accessor, AlertType const alertType);
    };
}

ANALYSE_FILE_END
