/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 Mikolaj Wielgus
 * Copyright (C) 2022 CERN
 * Copyright (C) 2022 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * https://www.gnu.org/licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */
#include <lib_symbol.h>
#include <sch_symbol.h>
#include <confirm.h>
#include <string_utils.h>

#include <sim/sim_model.h>
#include <sim/sim_model_behavioral.h>
#include <sim/sim_model_ideal.h>
#include <sim/sim_model_l_mutual.h>
#include <sim/sim_model_ngspice.h>
#include <sim/sim_model_r_pot.h>
#include <sim/sim_model_kibis.h>
#include <sim/sim_model_source.h>
#include <sim/sim_model_raw_spice.h>
#include <sim/sim_model_subckt.h>
#include <sim/sim_model_switch.h>
#include <sim/sim_model_tline.h>
#include <sim/sim_model_xspice.h>

#include <sim/sim_library_kibis.h>

#include <boost/algorithm/string/case_conv.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/algorithm/string.hpp>
#include <fmt/core.h>
#include <pegtl.hpp>
#include <pegtl/contrib/parse_tree.hpp>

#include <iterator>
#include "wx/regex.h"

using TYPE = SIM_MODEL::TYPE;


SIM_MODEL::DEVICE_INFO SIM_MODEL::DeviceInfo( DEVICE_T aDeviceType )
{
    switch( aDeviceType )
    {
        case DEVICE_T::NONE:      return { "",       "",                  true };
        case DEVICE_T::R:         return { "R",      "Resistor",          true };
        case DEVICE_T::C:         return { "C",      "Capacitor",         true };
        case DEVICE_T::L:         return { "L",      "Inductor",          true };
        case DEVICE_T::TLINE:     return { "TLINE",  "Transmission Line", true };
        case DEVICE_T::SW:        return { "SW",     "Switch",            true };

        case DEVICE_T::D:         return { "D",      "Diode",             true };
        case DEVICE_T::NPN:       return { "NPN",    "NPN BJT",           true };
        case DEVICE_T::PNP:       return { "PNP",    "PNP BJT",           true };

        case DEVICE_T::NJFET:     return { "NJFET",  "N-channel JFET",    true };
        case DEVICE_T::PJFET:     return { "PJFET",  "P-channel JFET",    true };

        case DEVICE_T::NMOS:      return { "NMOS",   "N-channel MOSFET",  true };
        case DEVICE_T::PMOS:      return { "PMOS",   "P-channel MOSFET",  true };
        case DEVICE_T::NMES:      return { "NMES",   "N-channel MESFET",  true };
        case DEVICE_T::PMES:      return { "PMES",   "P-channel MESFET",  true };

        case DEVICE_T::V:         return { "V",      "Voltage Source",    true };
        case DEVICE_T::I:         return { "I",      "Current Source",    true };

        case DEVICE_T::KIBIS:     return { "IBIS",  "IBIS Model",         false };

        case DEVICE_T::SUBCKT:    return { "SUBCKT", "Subcircuit",        false };
        case DEVICE_T::XSPICE:    return { "XSPICE", "XSPICE Code Model", true };
        case DEVICE_T::SPICE:     return { "SPICE",  "Raw Spice Element", true };
        case DEVICE_T::_ENUM_END: break;
    }

    wxFAIL;
    return {};
}


SIM_MODEL::INFO SIM_MODEL::TypeInfo( TYPE aType )
{
    switch( aType )
    {
    case TYPE::NONE:                 return { DEVICE_T::NONE,   "",               ""                           };

    case TYPE::R:                    return { DEVICE_T::R,      "",               "Ideal"                      };
    case TYPE::R_POT:                return { DEVICE_T::R,      "POT",            "Potentiometer"              };
    case TYPE::R_BEHAVIORAL:         return { DEVICE_T::R,      "=",              "Behavioral"                 };

    case TYPE::C:                    return { DEVICE_T::C,      "",               "Ideal"                      };
    case TYPE::C_BEHAVIORAL:         return { DEVICE_T::C,      "=",              "Behavioral"                 };

    case TYPE::L:                    return { DEVICE_T::L,      "",               "Ideal"                      };
    case TYPE::L_MUTUAL:             return { DEVICE_T::L,      "MUTUAL",         "Mutual"                     };
    case TYPE::L_BEHAVIORAL:         return { DEVICE_T::L,      "=",              "Behavioral"                 };

    case TYPE::TLINE_Z0:             return { DEVICE_T::TLINE,  "",               "Characteristic impedance"   };
    case TYPE::TLINE_RLGC:           return { DEVICE_T::TLINE,  "RLGC",           "RLGC"                       };

    case TYPE::SW_V:                 return { DEVICE_T::SW,     "V",              "Voltage-controlled"         };
    case TYPE::SW_I:                 return { DEVICE_T::SW,     "I",              "Current-controlled"         };

    case TYPE::D:                    return { DEVICE_T::D,      "",               ""                           };

    case TYPE::NPN_VBIC:             return { DEVICE_T::NPN,    "VBIC",           "VBIC"                       };
    case TYPE::PNP_VBIC:             return { DEVICE_T::PNP,    "VBIC",           "VBIC"                       };
    case TYPE::NPN_GUMMELPOON:       return { DEVICE_T::NPN,    "GUMMELPOON",     "Gummel-Poon"                };
    case TYPE::PNP_GUMMELPOON:       return { DEVICE_T::PNP,    "GUMMELPOON",     "Gummel-Poon"                };
    //case TYPE::BJT_MEXTRAM:          return {};
    case TYPE::NPN_HICUM2:           return { DEVICE_T::NPN,    "HICUML2",        "HICUM level 2"              };
    case TYPE::PNP_HICUM2:           return { DEVICE_T::PNP,    "HICUML2",        "HICUM level 2"              };
    //case TYPE::BJT_HICUM_L0:         return {};

    case TYPE::NJFET_SHICHMANHODGES: return { DEVICE_T::NJFET,  "SHICHMANHODGES", "Shichman-Hodges"            };
    case TYPE::PJFET_SHICHMANHODGES: return { DEVICE_T::PJFET,  "SHICHMANHODGES", "Shichman-Hodges"            };
    case TYPE::NJFET_PARKERSKELLERN: return { DEVICE_T::NJFET,  "PARKERSKELLERN", "Parker-Skellern"            };
    case TYPE::PJFET_PARKERSKELLERN: return { DEVICE_T::PJFET,  "PARKERSKELLERN", "Parker-Skellern"            };

    case TYPE::NMES_STATZ:           return { DEVICE_T::NMES,   "STATZ",          "Statz"                      };
    case TYPE::PMES_STATZ:           return { DEVICE_T::PMES,   "STATZ",          "Statz"                      };
    case TYPE::NMES_YTTERDAL:        return { DEVICE_T::NMES,   "YTTERDAL",       "Ytterdal"                   };
    case TYPE::PMES_YTTERDAL:        return { DEVICE_T::PMES,   "YTTERDAL",       "Ytterdal"                   };
    case TYPE::NMES_HFET1:           return { DEVICE_T::NMES,   "HFET1",          "HFET1"                      };
    case TYPE::PMES_HFET1:           return { DEVICE_T::PMES,   "HFET1",          "HFET1"                      };
    case TYPE::NMES_HFET2:           return { DEVICE_T::NMES,   "HFET2",          "HFET2"                      };
    case TYPE::PMES_HFET2:           return { DEVICE_T::PMES,   "HFET2",          "HFET2"                      };

    case TYPE::NMOS_VDMOS:           return { DEVICE_T::NMOS,   "VDMOS",          "VDMOS"                      };
    case TYPE::PMOS_VDMOS:           return { DEVICE_T::PMOS,   "VDMOS",          "VDMOS"                      };
    case TYPE::NMOS_MOS1:            return { DEVICE_T::NMOS,   "MOS1",           "Classical quadratic (MOS1)" };
    case TYPE::PMOS_MOS1:            return { DEVICE_T::PMOS,   "MOS1",           "Classical quadratic (MOS1)" };
    case TYPE::NMOS_MOS2:            return { DEVICE_T::NMOS,   "MOS2",           "Grove-Frohman (MOS2)"       };
    case TYPE::PMOS_MOS2:            return { DEVICE_T::PMOS,   "MOS2",           "Grove-Frohman (MOS2)"       };
    case TYPE::NMOS_MOS3:            return { DEVICE_T::NMOS,   "MOS3",           "MOS3"                       };
    case TYPE::PMOS_MOS3:            return { DEVICE_T::PMOS,   "MOS3",           "MOS3"                       };
    case TYPE::NMOS_BSIM1:           return { DEVICE_T::NMOS,   "BSIM1",          "BSIM1"                      };
    case TYPE::PMOS_BSIM1:           return { DEVICE_T::PMOS,   "BSIM1",          "BSIM1"                      };
    case TYPE::NMOS_BSIM2:           return { DEVICE_T::NMOS,   "BSIM2",          "BSIM2"                      };
    case TYPE::PMOS_BSIM2:           return { DEVICE_T::PMOS,   "BSIM2",          "BSIM2"                      };
    case TYPE::NMOS_MOS6:            return { DEVICE_T::NMOS,   "MOS6",           "MOS6"                       };
    case TYPE::PMOS_MOS6:            return { DEVICE_T::PMOS,   "MOS6",           "MOS6"                       };
    case TYPE::NMOS_BSIM3:           return { DEVICE_T::NMOS,   "BSIM3",          "BSIM3"                      };
    case TYPE::PMOS_BSIM3:           return { DEVICE_T::PMOS,   "BSIM3",          "BSIM3"                      };
    case TYPE::NMOS_MOS9:            return { DEVICE_T::NMOS,   "MOS9",           "MOS9"                       };
    case TYPE::PMOS_MOS9:            return { DEVICE_T::PMOS,   "MOS9",           "MOS9"                       };
    case TYPE::NMOS_B4SOI:           return { DEVICE_T::NMOS,   "B4SOI",          "BSIM4 SOI (B4SOI)"          };
    case TYPE::PMOS_B4SOI:           return { DEVICE_T::PMOS,   "B4SOI",          "BSIM4 SOI (B4SOI)"          };
    case TYPE::NMOS_BSIM4:           return { DEVICE_T::NMOS,   "BSIM4",          "BSIM4"                      };
    case TYPE::PMOS_BSIM4:           return { DEVICE_T::PMOS,   "BSIM4",          "BSIM4"                      };
    //case TYPE::NMOS_EKV2_6:          return {};
    //case TYPE::PMOS_EKV2_6:          return {};
    //case TYPE::NMOS_PSP:             return {};
    //case TYPE::PMOS_PSP:             return {};
    case TYPE::NMOS_B3SOIFD:         return { DEVICE_T::NMOS,   "B3SOIFD",        "B3SOIFD (BSIM3 FD-SOI)"     };
    case TYPE::PMOS_B3SOIFD:         return { DEVICE_T::PMOS,   "B3SOIFD",        "B3SOIFD (BSIM3 FD-SOI)"     };
    case TYPE::NMOS_B3SOIDD:         return { DEVICE_T::NMOS,   "B3SOIDD",        "B3SOIDD (BSIM3 SOI)"        };
    case TYPE::PMOS_B3SOIDD:         return { DEVICE_T::PMOS,   "B3SOIDD",        "B3SOIDD (BSIM3 SOI)"        };
    case TYPE::NMOS_B3SOIPD:         return { DEVICE_T::NMOS,   "B3SOIPD",        "B3SOIPD (BSIM3 PD-SOI)"     };
    case TYPE::PMOS_B3SOIPD:         return { DEVICE_T::PMOS,   "B3SOIPD",        "B3SOIPD (BSIM3 PD-SOI)"     };
    //case TYPE::NMOS_STAG:            return {};
    //case TYPE::PMOS_STAG:            return {};
    case TYPE::NMOS_HISIM2:          return { DEVICE_T::NMOS,   "HISIM2",         "HiSIM2"                     };
    case TYPE::PMOS_HISIM2:          return { DEVICE_T::PMOS,   "HISIM2",         "HiSIM2"                     };
    case TYPE::NMOS_HISIMHV1:        return { DEVICE_T::NMOS,   "HISIMHV1",       "HiSIM_HV1"                  };
    case TYPE::PMOS_HISIMHV1:        return { DEVICE_T::PMOS,   "HISIMHV1",       "HiSIM_HV1"                  };
    case TYPE::NMOS_HISIMHV2:        return { DEVICE_T::NMOS,   "HISIMHV2",       "HiSIM_HV2"                  };
    case TYPE::PMOS_HISIMHV2:        return { DEVICE_T::PMOS,   "HISIMHV2",       "HiSIM_HV2"                  };

    case TYPE::V:                    return { DEVICE_T::V,      "",               "DC",                        };
    case TYPE::V_SIN:                return { DEVICE_T::V,      "SIN",            "Sine"                       };
    case TYPE::V_PULSE:              return { DEVICE_T::V,      "PULSE",          "Pulse"                      };
    case TYPE::V_EXP:                return { DEVICE_T::V,      "EXP",            "Exponential"                };
    /*case TYPE::V_SFAM:               return { DEVICE_TYPE::V,      "SFAM",           "Single-frequency AM"        };
    case TYPE::V_SFFM:               return { DEVICE_TYPE::V,      "SFFM",           "Single-frequency FM"        };*/
    case TYPE::V_PWL:                return { DEVICE_T::V,      "PWL",            "Piecewise linear"           };
    case TYPE::V_WHITENOISE:         return { DEVICE_T::V,      "WHITENOISE",     "White noise"                };
    case TYPE::V_PINKNOISE:          return { DEVICE_T::V,      "PINKNOISE",      "Pink noise (1/f)"           };
    case TYPE::V_BURSTNOISE:         return { DEVICE_T::V,      "BURSTNOISE",     "Burst noise"                };
    case TYPE::V_RANDUNIFORM:        return { DEVICE_T::V,      "RANDUNIFORM",    "Random uniform"             };
    case TYPE::V_RANDNORMAL:         return { DEVICE_T::V,      "RANDNORMAL",     "Random normal"              };
    case TYPE::V_RANDEXP:            return { DEVICE_T::V,      "RANDEXP",        "Random exponential"         };
    //case TYPE::V_RANDPOISSON:        return { DEVICE_TYPE::V,      "RANDPOISSON",    "Random Poisson"             };
    case TYPE::V_BEHAVIORAL:         return { DEVICE_T::V,      "=",              "Behavioral"                 };

    case TYPE::I:                    return { DEVICE_T::I,      "",               "DC",                        };
    case TYPE::I_SIN:                return { DEVICE_T::I,      "SIN",            "Sine"                       };
    case TYPE::I_PULSE:              return { DEVICE_T::I,      "PULSE",          "Pulse"                      };
    case TYPE::I_EXP:                return { DEVICE_T::I,      "EXP",            "Exponential"                };
    /*case TYPE::I_SFAM:               return { DEVICE_TYPE::I,      "SFAM",           "Single-frequency AM"        };
    case TYPE::I_SFFM:               return { DEVICE_TYPE::I,      "SFFM",           "Single-frequency FM"        };*/
    case TYPE::I_PWL:                return { DEVICE_T::I,      "PWL",            "Piecewise linear"           };
    case TYPE::I_WHITENOISE:         return { DEVICE_T::I,      "WHITENOISE",     "White noise"                };
    case TYPE::I_PINKNOISE:          return { DEVICE_T::I,      "PINKNOISE",      "Pink noise (1/f)"           };
    case TYPE::I_BURSTNOISE:         return { DEVICE_T::I,      "BURSTNOISE",     "Burst noise"                };
    case TYPE::I_RANDUNIFORM:        return { DEVICE_T::I,      "RANDUNIFORM",    "Random uniform"             };
    case TYPE::I_RANDNORMAL:         return { DEVICE_T::I,      "RANDNORMAL",     "Random normal"              };
    case TYPE::I_RANDEXP:            return { DEVICE_T::I,      "RANDEXP",        "Random exponential"         };
    //case TYPE::I_RANDPOISSON:        return { DEVICE_TYPE::I,      "RANDPOISSON",    "Random Poisson"             };
    case TYPE::I_BEHAVIORAL:         return { DEVICE_T::I,      "=",              "Behavioral"                 };

    case TYPE::SUBCKT:               return { DEVICE_T::SUBCKT, "",               ""                           };
    case TYPE::XSPICE:               return { DEVICE_T::XSPICE, "",               ""                           };

    case TYPE::KIBIS_DEVICE:         return { DEVICE_T::KIBIS,  "DEVICE",         "Device"                     };
    case TYPE::KIBIS_DRIVER_DC:      return { DEVICE_T::KIBIS,  "DCDRIVER",       "DC driver"                  };
    case TYPE::KIBIS_DRIVER_RECT:    return { DEVICE_T::KIBIS,  "RECTDRIVER",     "Rectangular wave driver"    };
    case TYPE::KIBIS_DRIVER_PRBS:    return { DEVICE_T::KIBIS,  "PRBSDRIVER",     "PRBS driver"                };

    case TYPE::RAWSPICE:             return { DEVICE_T::SPICE,  "",               ""                           };

    case TYPE::_ENUM_END:            break;
    }

    wxFAIL;
    return {};
}


SIM_MODEL::SPICE_INFO SIM_MODEL::SpiceInfo( TYPE aType )
{
    switch( aType )
    {
    case TYPE::R:                    return { "R", ""             };
    case TYPE::R_POT:                return { "A", ""             };
    case TYPE::R_BEHAVIORAL:         return { "R", "",            "",        "0",  false, true   };

    case TYPE::C:                    return { "C", ""             };
    case TYPE::C_BEHAVIORAL:         return { "C", "",            "",        "0",  false, true   };

    case TYPE::L:                    return { "L", ""             };
    case TYPE::L_MUTUAL:             return { "K", ""             };
    case TYPE::L_BEHAVIORAL:         return { "L", "",            "",        "0",  false, true   };

    //case TYPE::TLINE_Z0:             return { "T"  };
    case TYPE::TLINE_Z0:             return { "O", "LTRA"         };
    case TYPE::TLINE_RLGC:           return { "O", "LTRA"         };

    case TYPE::SW_V:                 return { "S", "SW"           };
    case TYPE::SW_I:                 return { "W", "CSW"          };

    case TYPE::D:                    return { "D", "D"            };

    case TYPE::NPN_VBIC:             return { "Q", "NPN",         "",        "4"   };
    case TYPE::PNP_VBIC:             return { "Q", "PNP",         "",        "4"   };
    case TYPE::NPN_GUMMELPOON:       return { "Q", "NPN",         "",        "1",  true   };
    case TYPE::PNP_GUMMELPOON:       return { "Q", "PNP",         "",        "1",  true   };
    case TYPE::NPN_HICUM2:           return { "Q", "NPN",         "",        "8"   };
    case TYPE::PNP_HICUM2:           return { "Q", "PNP",         "",        "8"   };

    case TYPE::NJFET_SHICHMANHODGES: return { "M", "NJF",         "",        "1"   };
    case TYPE::PJFET_SHICHMANHODGES: return { "M", "PJF",         "",        "1"   };
    case TYPE::NJFET_PARKERSKELLERN: return { "M", "NJF",         "",        "2"   };
    case TYPE::PJFET_PARKERSKELLERN: return { "M", "PJF",         "",        "2"   };

    case TYPE::NMES_STATZ:           return { "Z", "NMF",         "",        "1"   };
    case TYPE::PMES_STATZ:           return { "Z", "PMF",         "",        "1"   };
    case TYPE::NMES_YTTERDAL:        return { "Z", "NMF",         "",        "2"   };
    case TYPE::PMES_YTTERDAL:        return { "Z", "PMF",         "",        "2"   };
    case TYPE::NMES_HFET1:           return { "Z", "NMF",         "",        "5"   };
    case TYPE::PMES_HFET1:           return { "Z", "PMF",         "",        "5"   };
    case TYPE::NMES_HFET2:           return { "Z", "NMF",         "",        "6"   };
    case TYPE::PMES_HFET2:           return { "Z", "PMF",         "",        "6"   };

    case TYPE::NMOS_VDMOS:           return { "M", "VDMOS NCHAN", };
    case TYPE::PMOS_VDMOS:           return { "M", "VDMOS PCHAN", };
    case TYPE::NMOS_MOS1:            return { "M", "NMOS",        "",        "1"   };
    case TYPE::PMOS_MOS1:            return { "M", "PMOS",        "",        "1"   };
    case TYPE::NMOS_MOS2:            return { "M", "NMOS",        "",        "2"   };
    case TYPE::PMOS_MOS2:            return { "M", "PMOS",        "",        "2"   };
    case TYPE::NMOS_MOS3:            return { "M", "NMOS",        "",        "3"   };
    case TYPE::PMOS_MOS3:            return { "M", "PMOS",        "",        "3"   };
    case TYPE::NMOS_BSIM1:           return { "M", "NMOS",        "",        "4"   };
    case TYPE::PMOS_BSIM1:           return { "M", "PMOS",        "",        "4"   };
    case TYPE::NMOS_BSIM2:           return { "M", "NMOS",        "",        "5"   };
    case TYPE::PMOS_BSIM2:           return { "M", "PMOS",        "",        "5"   };
    case TYPE::NMOS_MOS6:            return { "M", "NMOS",        "",        "6"   };
    case TYPE::PMOS_MOS6:            return { "M", "PMOS",        "",        "6"   };
    case TYPE::NMOS_BSIM3:           return { "M", "NMOS",        "",        "8"   };
    case TYPE::PMOS_BSIM3:           return { "M", "PMOS",        "",        "8"   };
    case TYPE::NMOS_MOS9:            return { "M", "NMOS",        "",        "9"   };
    case TYPE::PMOS_MOS9:            return { "M", "PMOS",        "",        "9"   };
    case TYPE::NMOS_B4SOI:           return { "M", "NMOS",        "",        "10"  };
    case TYPE::PMOS_B4SOI:           return { "M", "PMOS",        "",        "10"  };
    case TYPE::NMOS_BSIM4:           return { "M", "NMOS",        "",        "14"  };
    case TYPE::PMOS_BSIM4:           return { "M", "PMOS",        "",        "14"  };
    //case TYPE::NMOS_EKV2_6:          return {};
    //case TYPE::PMOS_EKV2_6:          return {};
    //case TYPE::NMOS_PSP:             return {};
    //case TYPE::PMOS_PSP:             return {};
    case TYPE::NMOS_B3SOIFD:         return { "M", "NMOS",        "",        "55"  };
    case TYPE::PMOS_B3SOIFD:         return { "M", "PMOS",        "",        "55"  };
    case TYPE::NMOS_B3SOIDD:         return { "M", "NMOS",        "",        "56"  };
    case TYPE::PMOS_B3SOIDD:         return { "M", "PMOS",        "",        "56"  };
    case TYPE::NMOS_B3SOIPD:         return { "M", "NMOS",        "",        "57"  };
    case TYPE::PMOS_B3SOIPD:         return { "M", "PMOS",        "",        "57"  };
    //case TYPE::NMOS_STAG:            return {};
    //case TYPE::PMOS_STAG:            return {};
    case TYPE::NMOS_HISIM2:          return { "M", "NMOS",        "",        "68"  };
    case TYPE::PMOS_HISIM2:          return { "M", "PMOS",        "",        "68"  };
    case TYPE::NMOS_HISIMHV1:        return { "M", "NMOS",        "",        "73", true,  false, "1.2.4" };
    case TYPE::PMOS_HISIMHV1:        return { "M", "PMOS",        "",        "73", true,  false, "1.2.4" };
    case TYPE::NMOS_HISIMHV2:        return { "M", "NMOS",        "",        "73", true,  false, "2.2.0" };
    case TYPE::PMOS_HISIMHV2:        return { "M", "PMOS",        "",        "73", true,  false, "2.2.0" };

    case TYPE::V:                    return { "V", ""             };
    case TYPE::V_SIN:                return { "V", "",            "SIN"      };
    case TYPE::V_PULSE:              return { "V", "",            "PULSE"    };
    case TYPE::V_EXP:                return { "V", "",            "EXP"      };
    /*case TYPE::V_SFAM:               return { "V", "",       "AM"       };
    case TYPE::V_SFFM:               return { "V", "",       "SFFM"     };*/
    case TYPE::V_PWL:                return { "V", "",            "PWL"      };
    case TYPE::V_WHITENOISE:         return { "V", "",            "TRNOISE"  };
    case TYPE::V_PINKNOISE:          return { "V", "",            "TRNOISE"  };
    case TYPE::V_BURSTNOISE:         return { "V", "",            "TRNOISE"  };
    case TYPE::V_RANDUNIFORM:        return { "V", "",            "TRRANDOM" };
    case TYPE::V_RANDNORMAL:         return { "V", "",            "TRRANDOM" };
    case TYPE::V_RANDEXP:            return { "V", "",            "TRRANDOM" };
    //case TYPE::V_RANDPOISSON:        return { "V", "",       "TRRANDOM" };
    case TYPE::V_BEHAVIORAL:         return { "B"  };

    case TYPE::I:                    return { "I", ""             };
    case TYPE::I_PULSE:              return { "I", "",            "PULSE"    };
    case TYPE::I_SIN:                return { "I", "",            "SIN"      };
    case TYPE::I_EXP:                return { "I", "",            "EXP"      };
    /*case TYPE::I_SFAM:               return { "V", "",       "AM"       };
    case TYPE::I_SFFM:               return { "V", "",       "SFFM"     };*/
    case TYPE::I_PWL:                return { "I", "",            "PWL"      };
    case TYPE::I_WHITENOISE:         return { "I", "",            "TRNOISE"  };
    case TYPE::I_PINKNOISE:          return { "I", "",            "TRNOISE"  };
    case TYPE::I_BURSTNOISE:         return { "I", "",            "TRNOISE"  };
    case TYPE::I_RANDUNIFORM:        return { "I", "",            "TRRANDOM" };
    case TYPE::I_RANDNORMAL:         return { "I", "",            "TRRANDOM" };
    case TYPE::I_RANDEXP:            return { "I", "",            "TRRANDOM" };
    //case TYPE::I_RANDPOISSON:        return { "I", "",       "TRRANDOM" };
    case TYPE::I_BEHAVIORAL:         return { "B"  };

    case TYPE::SUBCKT:               return { "X"  };
    case TYPE::XSPICE:               return { "A"  };

    case TYPE::KIBIS_DEVICE:         return { "X"  };
    case TYPE::KIBIS_DRIVER_DC:      return { "X"  };
    case TYPE::KIBIS_DRIVER_RECT:    return { "X"  };
    case TYPE::KIBIS_DRIVER_PRBS:    return { "X"  };

    case TYPE::NONE:
    case TYPE::RAWSPICE:
        return {};

    case TYPE::_ENUM_END:
        break;
    }

    wxFAIL;
    return {};
}


template TYPE SIM_MODEL::ReadTypeFromFields( const std::vector<SCH_FIELD>& aFields,
                                             int aSymbolPinCount );
template TYPE SIM_MODEL::ReadTypeFromFields( const std::vector<LIB_FIELD>& aFields,
                                             int aSymbolPinCount );

template <typename T>
TYPE SIM_MODEL::ReadTypeFromFields( const std::vector<T>& aFields, int aSymbolPinCount )
{
    std::string deviceTypeFieldValue = GetFieldValue( &aFields, DEVICE_TYPE_FIELD );
    std::string typeFieldValue = GetFieldValue( &aFields, TYPE_FIELD );

    if( deviceTypeFieldValue != "" )
    {
        for( TYPE type : TYPE_ITERATOR() )
        {
            if( typeFieldValue == TypeInfo( type ).fieldValue )
            {
                if( deviceTypeFieldValue == DeviceInfo( TypeInfo( type ).deviceType ).fieldValue )
                    return type;
            }
        }
    }

    if( typeFieldValue != "" )
        return TYPE::NONE;

    // No type information. Look for legacy (pre-V7) fields.

    TYPE typeFromLegacyFields = InferTypeFromLegacyFields( aFields );

    if( typeFromLegacyFields != TYPE::NONE )
        return typeFromLegacyFields;

    return TYPE::NONE;
}


template <typename T>
TYPE SIM_MODEL::InferTypeFromLegacyFields( const std::vector<T>& aFields )
{
    if( GetFieldValue( &aFields, SIM_MODEL_RAW_SPICE::LEGACY_TYPE_FIELD ) != ""
        || GetFieldValue( &aFields, SIM_MODEL_RAW_SPICE::LEGACY_MODEL_FIELD ) != ""
        || GetFieldValue( &aFields, SIM_MODEL_RAW_SPICE::LEGACY_ENABLED_FIELD ) != ""
        || GetFieldValue( &aFields, SIM_MODEL_RAW_SPICE::LEGACY_LIB_FIELD ) != "" )
    {
        return TYPE::RAWSPICE;
    }
    else
    {
        return TYPE::NONE;
    }
}


template <typename T>
void SIM_MODEL::ReadDataFields( unsigned aSymbolPinCount, const std::vector<T>* aFields )
{
    doReadDataFields( aSymbolPinCount, aFields );
}


template <>
void SIM_MODEL::ReadDataFields( unsigned aSymbolPinCount, const std::vector<SCH_FIELD>* aFields )
{
    ReadDataSchFields( aSymbolPinCount, aFields );
}


template <>
void SIM_MODEL::ReadDataFields( unsigned aSymbolPinCount, const std::vector<LIB_FIELD>* aFields )
{
    ReadDataLibFields( aSymbolPinCount, aFields );
}


void SIM_MODEL::ReadDataSchFields( unsigned aSymbolPinCount, const std::vector<SCH_FIELD>* aFields )
{
    doReadDataFields( aSymbolPinCount, aFields );
}


void SIM_MODEL::ReadDataLibFields( unsigned aSymbolPinCount, const std::vector<LIB_FIELD>* aFields )
{
    doReadDataFields( aSymbolPinCount, aFields );
}


template <typename T>
void SIM_MODEL::WriteFields( std::vector<T>& aFields ) const
{
    doWriteFields( aFields );
}


template <>
void SIM_MODEL::WriteFields( std::vector<SCH_FIELD>& aFields ) const
{
    WriteDataSchFields( aFields );
}


template <>
void SIM_MODEL::WriteFields( std::vector<LIB_FIELD>& aFields ) const
{
    WriteDataLibFields( aFields );
}


void SIM_MODEL::WriteDataSchFields( std::vector<SCH_FIELD>& aFields ) const
{
    doWriteFields( aFields );
}


void SIM_MODEL::WriteDataLibFields( std::vector<LIB_FIELD>& aFields ) const
{
    doWriteFields( aFields );
}


std::unique_ptr<SIM_MODEL> SIM_MODEL::Create( TYPE aType, unsigned aSymbolPinCount )
{
    std::unique_ptr<SIM_MODEL> model = Create( aType );

    // Passing nullptr to ReadDataFields will make it act as if all fields were empty.
    model->ReadDataFields( aSymbolPinCount, static_cast<const std::vector<void>*>( nullptr ) );
    return model;
}


std::unique_ptr<SIM_MODEL> SIM_MODEL::Create( const SIM_MODEL& aBaseModel, unsigned aSymbolPinCount )
{
    std::unique_ptr<SIM_MODEL> model = Create( aBaseModel.GetType() );

    model->SetBaseModel( aBaseModel );
    model->ReadDataFields( aSymbolPinCount, static_cast<const std::vector<void>*>( nullptr ) );
    return model;
}


template <typename T>
std::unique_ptr<SIM_MODEL> SIM_MODEL::Create( const SIM_MODEL& aBaseModel, unsigned aSymbolPinCount,
                                              const std::vector<T>& aFields )
{
    TYPE type = ReadTypeFromFields( aFields, aSymbolPinCount );

    // If the model has a specified type, it takes priority over the type of its base class.
    if( type == TYPE::NONE )
        type = aBaseModel.GetType();

    std::unique_ptr<SIM_MODEL> model = Create( type );

    model->SetBaseModel( aBaseModel );
    model->ReadDataFields( aSymbolPinCount, &aFields );
    return model;
}

template std::unique_ptr<SIM_MODEL> SIM_MODEL::Create( const SIM_MODEL& aBaseModel,
                                                       unsigned aSymbolPinCount,
                                                       const std::vector<SCH_FIELD>& aFields );

template std::unique_ptr<SIM_MODEL> SIM_MODEL::Create( const SIM_MODEL& aBaseModel,
                                                       unsigned aSymbolPinCount,
                                                       const std::vector<LIB_FIELD>& aFields );


template <typename T>
std::unique_ptr<SIM_MODEL> SIM_MODEL::Create( unsigned aSymbolPinCount,
                                              const std::vector<T>& aFields )
{
    TYPE type = ReadTypeFromFields( aFields, aSymbolPinCount );
    std::unique_ptr<SIM_MODEL> model = SIM_MODEL::Create( type );

    model->ReadDataFields( aSymbolPinCount, &aFields );
    return model;
}

template std::unique_ptr<SIM_MODEL> SIM_MODEL::Create( unsigned aSymbolPinCount,
                                                       const std::vector<SCH_FIELD>& aFields );
template std::unique_ptr<SIM_MODEL> SIM_MODEL::Create( unsigned aSymbolPinCount,
                                                       const std::vector<LIB_FIELD>& aFields );


template <typename T>
std::string SIM_MODEL::GetFieldValue( const std::vector<T>* aFields, const std::string& aFieldName )
{
    static_assert( std::is_same<T, SCH_FIELD>::value || std::is_same<T, LIB_FIELD>::value );

    if( !aFields )
        return ""; // Should not happen, T=void specialization will be called instead.

    auto it = std::find_if( aFields->begin(), aFields->end(),
                            [aFieldName]( const T& field )
                            {
                                return field.GetName() == aFieldName;
                            } );

    if( it != aFields->end() )
        return std::string( it->GetText().ToUTF8() );

    return "";
}


// This specialization is used when no fields are passed.
template <>
std::string SIM_MODEL::GetFieldValue( const std::vector<void>* aFields, const std::string& aFieldName )
{
    return "";
}


template <typename T>
void SIM_MODEL::SetFieldValue( std::vector<T>& aFields, const std::string& aFieldName,
                               const std::string& aValue )
{
    static_assert( std::is_same<T, SCH_FIELD>::value || std::is_same<T, LIB_FIELD>::value );

    auto fieldIt = std::find_if( aFields.begin(), aFields.end(),
                                 [&]( const T& f )
                                 {
                                    return f.GetName() == aFieldName;
                                 } );

    if( fieldIt != aFields.end() )
    {
        if( aValue == "" )
            aFields.erase( fieldIt );
        else
            fieldIt->SetText( aValue );

        return;
    }

    if( aValue == "" )
        return;

    if constexpr( std::is_same<T, SCH_FIELD>::value )
    {
        wxASSERT( aFields.size() >= 1 );

        SCH_ITEM* parent = static_cast<SCH_ITEM*>( aFields.at( 0 ).GetParent() );
        aFields.emplace_back( wxPoint(), aFields.size(), parent, aFieldName );
    }
    else if constexpr( std::is_same<T, LIB_FIELD>::value )
        aFields.emplace_back( aFields.size(), aFieldName );

    aFields.back().SetText( aValue );
}


SIM_MODEL::~SIM_MODEL() = default;


void SIM_MODEL::AddPin( const PIN& aPin )
{
    m_pins.push_back( aPin );
}


void SIM_MODEL::ClearPins()
{
    m_pins.clear();
}


int SIM_MODEL::FindModelPinIndex( const std::string& aSymbolPinNumber )
{
    for( int modelPinIndex = 0; modelPinIndex < GetPinCount(); ++modelPinIndex )
    {
        if( GetPin( modelPinIndex ).symbolPinNumber == aSymbolPinNumber )
            return modelPinIndex;
    }

    return PIN::NOT_CONNECTED;
}


void SIM_MODEL::AddParam( const PARAM::INFO& aInfo, bool aIsOtherVariant )
{
    m_params.emplace_back( aInfo, aIsOtherVariant );

    // Enums are initialized with their default values.
    if( aInfo.enumValues.size() >= 1 )
        m_params.back().value->FromString( aInfo.defaultValue );
}


std::vector<std::reference_wrapper<const SIM_MODEL::PIN>> SIM_MODEL::GetPins() const
{
    std::vector<std::reference_wrapper<const PIN>> pins;

    for( int modelPinIndex = 0; modelPinIndex < GetPinCount(); ++modelPinIndex )
        pins.emplace_back( GetPin( modelPinIndex ) );

    return pins;
}

void SIM_MODEL::SetPinSymbolPinNumber( int aPinIndex, const std::string& aSymbolPinNumber )
{
    m_pins.at( aPinIndex ).symbolPinNumber = aSymbolPinNumber;
}


void SIM_MODEL::SetPinSymbolPinNumber( const std::string& aPinName,
                                       const std::string& aSymbolPinNumber )
{
    const std::vector<std::reference_wrapper<const PIN>> pins = GetPins();

    auto it = std::find_if( pins.begin(), pins.end(),
                            [aPinName]( const PIN& aPin )
                            {
                                return aPin.name == aPinName;
                            } );

    if( it == pins.end() )
    {
        THROW_IO_ERROR( wxString::Format( _( "Could not find a pin named '%s' in simulation model of type '%s'" ),
                                          aPinName,
                                          GetTypeInfo().fieldValue ) );
    }

    SetPinSymbolPinNumber( static_cast<int>( it - pins.begin() ), aSymbolPinNumber );
}


const SIM_MODEL::PARAM& SIM_MODEL::GetParam( unsigned aParamIndex ) const
{
    if( m_baseModel && m_params.at( aParamIndex ).value->ToString() == "" )
        return m_baseModel->GetParam( aParamIndex );
    else
        return m_params.at( aParamIndex );
}


const SIM_MODEL::PARAM* SIM_MODEL::FindParam( const std::string& aParamName ) const
{
    std::vector<std::reference_wrapper<const PARAM>> params = GetParams();

    auto it = std::find_if( params.begin(), params.end(),
                            [aParamName]( const PARAM& param )
                            {
                                return param.info.name == boost::to_lower_copy( aParamName );
                            } );

    if( it == params.end() )
        return nullptr;

    return &it->get();
}


std::vector<std::reference_wrapper<const SIM_MODEL::PARAM>> SIM_MODEL::GetParams() const
{
    std::vector<std::reference_wrapper<const PARAM>> params;

    for( int i = 0; i < GetParamCount(); ++i )
        params.emplace_back( GetParam( i ) );

    return params;
}


const SIM_MODEL::PARAM& SIM_MODEL::GetUnderlyingParam( unsigned aParamIndex ) const
{
    return m_params.at( aParamIndex );
}


const SIM_MODEL::PARAM& SIM_MODEL::GetBaseParam( unsigned aParamIndex ) const
{
    if( m_baseModel )
        return m_baseModel->GetParam( aParamIndex );
    else
        return m_params.at( aParamIndex );
}


void SIM_MODEL::SetParamValue( int aParamIndex, const SIM_VALUE& aValue )
{
    *m_params.at( aParamIndex ).value = aValue;
}


void SIM_MODEL::SetParamValue( int aParamIndex, const std::string& aValue,
                               SIM_VALUE::NOTATION aNotation )
{
    const SIM_VALUE& value = *GetParam( aParamIndex ).value;
    SetParamValue( aParamIndex, *SIM_VALUE::Create( value.GetType(), aValue, aNotation ) );
}


void SIM_MODEL::SetParamValue( const std::string& aParamName, const SIM_VALUE& aValue )
{
    std::vector<std::reference_wrapper<const PARAM>> params = GetParams();

    auto it = std::find_if( params.begin(), params.end(),
                            [aParamName]( const PARAM& param )
                            {
                                return param.info.name == boost::to_lower_copy( aParamName );
                            } );

    if( it == params.end() )
    {
        THROW_IO_ERROR( wxString::Format( _( "Could not find a parameter named '%s' in simulation model of type '%s'" ),
                                          aParamName,
                                          GetTypeInfo().fieldValue ) );
    }

    SetParamValue( static_cast<int>( it - params.begin() ), aValue );
}


void SIM_MODEL::SetParamValue( const std::string& aParamName, const std::string& aValue,
                               SIM_VALUE::NOTATION aNotation )
{
    const PARAM* param = FindParam( aParamName );

    if( !param )
    {
        THROW_IO_ERROR( wxString::Format( _( "Could not find a parameter named '%s' in simulation model of type '%s'" ),
                                          aParamName,
                                          GetTypeInfo().fieldValue ) );
    }

    const SIM_VALUE& value = *FindParam( aParamName )->value;
    SetParamValue( aParamName, *SIM_VALUE::Create( value.GetType(), aValue, aNotation ) );
}


bool SIM_MODEL::HasOverrides() const
{
    for( const PARAM& param : m_params )
    {
        if( param.value->ToString() != "" )
            return true;
    }

    return false;
}


bool SIM_MODEL::HasNonInstanceOverrides() const
{
    for( const PARAM& param : m_params )
    {
        if( !param.info.isInstanceParam && param.value->ToString() != "" )
            return true;
    }

    return false;
}


bool SIM_MODEL::HasSpiceNonInstanceOverrides() const
{
    for( const PARAM& param : m_params )
    {
        if( !param.info.isSpiceInstanceParam && param.value->ToString() != "" )
            return true;
    }

    return false;
}


std::unique_ptr<SIM_MODEL> SIM_MODEL::Create( TYPE aType )
{
    switch( aType )
    {
    case TYPE::R:
    case TYPE::C:
    case TYPE::L:
        return std::make_unique<SIM_MODEL_IDEAL>( aType );

    case TYPE::R_POT:
        return std::make_unique<SIM_MODEL_R_POT>();

    case TYPE::L_MUTUAL:
        return std::make_unique<SIM_MODEL_L_MUTUAL>();

    case TYPE::R_BEHAVIORAL:
    case TYPE::C_BEHAVIORAL:
    case TYPE::L_BEHAVIORAL:
    case TYPE::V_BEHAVIORAL:
    case TYPE::I_BEHAVIORAL:
        return std::make_unique<SIM_MODEL_BEHAVIORAL>( aType );

    case TYPE::TLINE_Z0:
    case TYPE::TLINE_RLGC:
        return std::make_unique<SIM_MODEL_TLINE>( aType );

    case TYPE::SW_V:
    case TYPE::SW_I:
        return std::make_unique<SIM_MODEL_SWITCH>( aType );

    case TYPE::V:
    case TYPE::I:
    case TYPE::V_SIN:
    case TYPE::I_SIN:
    case TYPE::V_PULSE:
    case TYPE::I_PULSE:
    case TYPE::V_EXP:
    case TYPE::I_EXP:
    /*case TYPE::V_SFAM:
    case TYPE::I_SFAM:
    case TYPE::V_SFFM:
    case TYPE::I_SFFM:*/
    case TYPE::V_PWL:
    case TYPE::I_PWL:
    case TYPE::V_WHITENOISE:
    case TYPE::I_WHITENOISE:
    case TYPE::V_PINKNOISE:
    case TYPE::I_PINKNOISE:
    case TYPE::V_BURSTNOISE:
    case TYPE::I_BURSTNOISE:
    case TYPE::V_RANDUNIFORM:
    case TYPE::I_RANDUNIFORM:
    case TYPE::V_RANDNORMAL:
    case TYPE::I_RANDNORMAL:
    case TYPE::V_RANDEXP:
    case TYPE::I_RANDEXP:
    //case TYPE::V_RANDPOISSON:
    //case TYPE::I_RANDPOISSON:
        return std::make_unique<SIM_MODEL_SOURCE>( aType );

    case TYPE::SUBCKT:
        return std::make_unique<SIM_MODEL_SUBCKT>();

    case TYPE::XSPICE:
        return std::make_unique<SIM_MODEL_XSPICE>( aType );

    case TYPE::KIBIS_DEVICE:
    case TYPE::KIBIS_DRIVER_DC:
    case TYPE::KIBIS_DRIVER_RECT:
    case TYPE::KIBIS_DRIVER_PRBS:
        return std::make_unique<SIM_MODEL_KIBIS>( aType );

    case TYPE::RAWSPICE:
        return std::make_unique<SIM_MODEL_RAW_SPICE>();

    default:
        return std::make_unique<SIM_MODEL_NGSPICE>( aType );
    }
}


SIM_MODEL::SIM_MODEL( TYPE aType ) :
        SIM_MODEL( aType, std::make_unique<SPICE_GENERATOR>( *this ),
                   std::make_unique<SIM_SERDE>( *this ) )
{
}


SIM_MODEL::SIM_MODEL( TYPE aType, std::unique_ptr<SPICE_GENERATOR> aSpiceGenerator ) :
        SIM_MODEL( aType, std::move( aSpiceGenerator ), std::make_unique<SIM_SERDE>( *this ) )
{
}


SIM_MODEL::SIM_MODEL( TYPE aType, std::unique_ptr<SPICE_GENERATOR> aSpiceGenerator,
                      std::unique_ptr<SIM_SERDE> aSerde ) :
        m_baseModel( nullptr ),
        m_serde( std::move( aSerde ) ),
        m_spiceGenerator( std::move( aSpiceGenerator ) ),
        m_type( aType ),
        m_isEnabled( true ),
        m_isStoredInValue( false )
{
}


void SIM_MODEL::CreatePins( unsigned aSymbolPinCount )
{
    // Default pin sequence: model pins are the same as symbol pins.
    // Excess model pins are set as Not Connected.
    // Note that intentionally nothing is added if `getPinNames()` returns an empty vector.

    // SIM_MODEL pins must be ordered by symbol pin numbers -- this is assumed by the code that
    // accesses them.

    for( unsigned modelPinIndex = 0; modelPinIndex < getPinNames().size(); ++modelPinIndex )
    {
        if( modelPinIndex < aSymbolPinCount )
            AddPin( { getPinNames().at( modelPinIndex ), fmt::format( "{}", modelPinIndex + 1 ) } );
        else
            AddPin( { getPinNames().at( modelPinIndex ), "" } );
    }
}


template <typename T>
void SIM_MODEL::doReadDataFields( unsigned aSymbolPinCount, const std::vector<T>* aFields )
{
    m_serde->ParseEnable( GetFieldValue( aFields, ENABLE_FIELD ) );

    CreatePins( aSymbolPinCount );
    m_serde->ParsePins( GetFieldValue( aFields, PINS_FIELD ) );

    std::string paramsField = GetFieldValue( aFields, PARAMS_FIELD );

    if( !m_serde->ParseParams( paramsField ) )
        m_serde->ParseValue( GetFieldValue( aFields, VALUE_FIELD ) );
}


template <typename T>
void SIM_MODEL::doWriteFields( std::vector<T>& aFields ) const
{
    SetFieldValue( aFields, DEVICE_TYPE_FIELD, m_serde->GenerateDevice() );
    SetFieldValue( aFields, TYPE_FIELD, m_serde->GenerateType() );

    SetFieldValue( aFields, ENABLE_FIELD, m_serde->GenerateEnable() );
    SetFieldValue( aFields, PINS_FIELD, m_serde->GeneratePins() );

    SetFieldValue( aFields, PARAMS_FIELD, m_serde->GenerateParams() );

    if( IsStoredInValue() )
        SetFieldValue( aFields, VALUE_FIELD, m_serde->GenerateValue() );
}


bool SIM_MODEL::requiresSpiceModelLine() const
{
    for( const PARAM& param : GetParams() )
    {
        if( !param.info.isSpiceInstanceParam )
            return true;
    }

    return false;
}


std::pair<wxString, wxString> SIM_MODEL::InferSimModel( const wxString& aPrefix,
                                                        const wxString& aValue )
{
    wxString spiceModelType;
    wxString spiceModelParams;

    if( !aValue.IsEmpty() )
    {
        if( aPrefix.StartsWith( wxT( "R" ) )
            || aPrefix.StartsWith( wxT( "L" ) )
            || aPrefix.StartsWith( wxT( "C" ) ) )
        {
            wxRegEx passiveVal( wxT( "^"
                                     "([0-9\\. ]+)"
                                     "([fFpPnNuUmMkKgGtTμµ𝛍𝜇𝝁 ]|M(e|E)(g|G))?"
                                     "([fFhHΩΩ𝛀𝛺𝝮]|ohm)?"
                                     "([-1-9 ]*)"
                                     "$" ) );

            if( passiveVal.Matches( aValue ) )
            {
                wxString valuePrefix( passiveVal.GetMatch( aValue, 1 ) );
                wxString valueUnits( passiveVal.GetMatch( aValue, 2 ) );
                wxString valueSuffix( passiveVal.GetMatch( aValue, 6 ) );

                if( valueUnits == wxT( "M" ) )
                    valueUnits = wxT( "Meg" );

                spiceModelParams = wxString::Format( wxT( "%s=\"%s%s\"" ),
                                                     aPrefix.Lower(),
                                                     valuePrefix,
                                                     valueUnits );
            }
            else
            {
                spiceModelType = wxT( "=" );
                spiceModelParams = wxString::Format( wxT( "%s=\"%s\"" ), aPrefix.Lower(), aValue );
            }
        }
    }

    return std::make_pair( spiceModelType, spiceModelParams );
}


template <typename T_symbol, typename T_field>
void SIM_MODEL::MigrateSimModel( T_symbol& aSymbol )
{
    if( aSymbol.FindField( SIM_MODEL::DEVICE_TYPE_FIELD )
        || aSymbol.FindField( SIM_MODEL::TYPE_FIELD )
        || aSymbol.FindField( SIM_MODEL::PINS_FIELD )
        || aSymbol.FindField( SIM_MODEL::PARAMS_FIELD ) )
    {
        // Has a V7 model field -- skip.
        return;
    }

    wxString prefix = aSymbol.GetPrefix();
    wxString value;

    // Yes, the Value field is always present, but Coverity doesn't know that...
    if( T_field* valueField = aSymbol.FindField( wxT( "Value" ) ) )
        value = valueField->GetText();

    wxString spiceType;
    wxString spiceModel;
    wxString spiceLib;
    wxString pinMap;

    if( aSymbol.FindField( wxT( "Spice_Primitive" ) )
        || aSymbol.FindField( wxT( "Spice_Node_Sequence" ) )
        || aSymbol.FindField( wxT( "Spice_Model" ) )
        || aSymbol.FindField( wxT( "Spice_Netlist_Enabled" ) )
        || aSymbol.FindField( wxT( "Spice_Lib_File" ) ) )
    {
        if( T_field* primitiveField = aSymbol.FindField( wxT( "Spice_Primitive" ) ) )
        {
            spiceType = primitiveField->GetText();
            aSymbol.RemoveField( primitiveField );
        }

        if( T_field* nodeSequenceField = aSymbol.FindField( wxT( "Spice_Node_Sequence" ) ) )
        {
            const wxString  delimiters( "{:,; }" );
            const wxString& nodeSequence = nodeSequenceField->GetText();

            if( nodeSequence != "" )
            {
                wxStringTokenizer tkz( nodeSequence, delimiters );

                for( long modelPinNumber = 1; tkz.HasMoreTokens(); ++modelPinNumber )
                {
                    long symbolPinNumber = 1;
                    tkz.GetNextToken().ToLong( &symbolPinNumber );

                    if( modelPinNumber != 1 )
                        pinMap.Append( " " );

                    pinMap.Append( wxString::Format( "%ld=%ld", symbolPinNumber, modelPinNumber ) );
                }
            }

            aSymbol.RemoveField( nodeSequenceField );
        }

        if( T_field* modelField = aSymbol.FindField( wxT( "Spice_Model" ) ) )
        {
            spiceModel = modelField->GetText();
            aSymbol.RemoveField( modelField );
        }
        else
        {
            spiceModel = value;
        }

        if( T_field* netlistEnabledField = aSymbol.FindField( wxT( "Spice_Netlist_Enabled" ) ) )
        {
            wxString netlistEnabled = netlistEnabledField->GetText().Lower();

            if( netlistEnabled.StartsWith( wxT( "0" ) )
                || netlistEnabled.StartsWith( wxT( "n" ) )
                || netlistEnabled.StartsWith( wxT( "f" ) ) )
            {
                T_field enableField( &aSymbol, aSymbol.GetFieldCount(), SIM_MODEL::ENABLE_FIELD );
            }
        }

        if( T_field* libFileField = aSymbol.FindField( wxT( "Spice_Lib_File" ) ) )
        {
            spiceLib = libFileField->GetText();
            aSymbol.RemoveField( libFileField );
        }
    }
    else if( prefix == wxT( "V" ) || prefix == wxT( "I" ) )
    {
        spiceModel = value;
    }
    else
    {
        // Auto convert some legacy fields used in the middle of 7.0 development...

        if( T_field* legacyDevice = aSymbol.FindField( wxT( "Sim_Type" ) ) )
        {
            legacyDevice->SetName( SIM_MODEL::TYPE_FIELD );
        }

        if( T_field* legacyDevice = aSymbol.FindField( wxT( "Sim_Device" ) ) )
        {
            legacyDevice->SetName( SIM_MODEL::DEVICE_TYPE_FIELD );
        }

        if( T_field* legacyPins = aSymbol.FindField( wxT( "Sim_Pins" ) ) )
        {
            // Migrate pins from array of indexes to name-value-pairs
            wxArrayString pinIndexes;
            wxString      pins;

            wxStringSplit( legacyPins->GetText(), pinIndexes, ' ' );

            if( SIM_MODEL::InferSimModel( prefix, value ).second.length() )
            {
                if( pinIndexes[0] == wxT( "2" ) )
                    pins = "1=- 2=+";
                else
                    pins = "1=+ 2=-";
            }
            else
            {
                for( unsigned ii = 0; ii < pinIndexes.size(); ++ii )
                {
                    if( ii > 0 )
                        pins.Append( wxS( " " ) );

                    pins.Append( wxString::Format( wxT( "%u=%s" ), ii + 1, pinIndexes[ ii ] ) );
                }
            }

            legacyPins->SetName( SIM_MODEL::PINS_FIELD );
            legacyPins->SetText( pins );
        }

        if( T_field* legacyPins = aSymbol.FindField( wxT( "Sim_Params" ) ) )
        {
            legacyPins->SetName( SIM_MODEL::PARAMS_FIELD );
        }

        return;
    }

    // Insert a plaintext model as a substitute.

    T_field deviceTypeField( &aSymbol, aSymbol.GetFieldCount(), SIM_MODEL::DEVICE_TYPE_FIELD );
    deviceTypeField.SetText( SIM_MODEL::DeviceInfo( SIM_MODEL::DEVICE_T::SPICE ).fieldValue );
    aSymbol.AddField( deviceTypeField );

    T_field paramsField( &aSymbol, aSymbol.GetFieldCount(), SIM_MODEL::PARAMS_FIELD );
    paramsField.SetText( wxString::Format( "type=\"%s\" model=\"%s\" lib=\"%s\"",
                                           spiceType, spiceModel, spiceLib ) );
    aSymbol.AddField( paramsField );

    // Legacy models by default get linear pin mapping.
    if( pinMap != "" )
    {
        T_field pinsField( &aSymbol, aSymbol.GetFieldCount(), SIM_MODEL::PINS_FIELD );

        pinsField.SetText( pinMap );
        aSymbol.AddField( pinsField );
    }
    else
    {
        wxString pins;

        for( unsigned ii = 0; ii < aSymbol.GetPinCount(); ++ii )
        {
            if( ii > 0 )
                pins.Append( wxS( " " ) );

            pins.Append( wxString::Format( wxT( "%u=%u" ), ii + 1, ii + 1 ) );
        }

        T_field pinsField( &aSymbol, aSymbol.GetFieldCount(), SIM_MODEL::PINS_FIELD );
        pinsField.SetText( pins );
        aSymbol.AddField( pinsField );
    }
}


template void SIM_MODEL::MigrateSimModel<SCH_SYMBOL, SCH_FIELD>( SCH_SYMBOL& aSymbol );
template void SIM_MODEL::MigrateSimModel<LIB_SYMBOL, LIB_FIELD>( LIB_SYMBOL& aSymbol );
