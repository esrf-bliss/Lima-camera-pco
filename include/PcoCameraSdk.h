//###########################################################################
// This file is part of LImA, a Library for Image Acquisition
//
// Copyright (C) : 2009-2024
// European Synchrotron Radiation Facility
// BP 220, Grenoble 38043
// FRANCE
//
// This is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.
//
// This software is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, see <http://www.gnu.org/licenses/>.
//###########################################################################

#ifndef PCOCAMERASDK_H
#define PCOCAMERASDK_H

//#ifndef __linux

#    define PCO_FN0(er, mg, fn)                                                \
        {                                                                      \
            mg = #fn;                                                          \
            er = PcoCheckError(__LINE__, __FILE__, fn(), #fn);                 \
        }
#    define PCO_FN1(er, mg, fn, x1)                                            \
        {                                                                      \
            mg = #fn;                                                          \
            er = PcoCheckError(__LINE__, __FILE__, fn((x1)), #fn);             \
        }
#    define PCO_FN2(er, mg, fn, x1, x2)                                        \
        {                                                                      \
            mg = #fn;                                                          \
            er = PcoCheckError(__LINE__, __FILE__, fn((x1), (x2)), #fn);       \
        }
#    define PCO_FN3(er, mg, fn, x1, x2, x3)                                    \
        {                                                                      \
            mg = #fn;                                                          \
            er = PcoCheckError(__LINE__, __FILE__, fn((x1), (x2), (x3)), #fn); \
        }
#    define PCO_FN4(er, mg, fn, x1, x2, x3, x4)                                \
        {                                                                      \
            mg = #fn;                                                          \
            er = PcoCheckError(__LINE__, __FILE__, fn((x1), (x2), (x3), (x4)), \
                               #fn);                                           \
        }
#    define PCO_FN5(er, mg, fn, x1, x2, x3, x4, x5)                            \
        {                                                                      \
            mg = #fn;                                                          \
            er = PcoCheckError(__LINE__, __FILE__,                             \
                               fn((x1), (x2), (x3), (x4), (x5)), #fn);         \
        }
#    define PCO_FN6(er, mg, fn, x1, x2, x3, x4, x5, x6)                        \
        {                                                                      \
            mg = #fn;                                                          \
            er = PcoCheckError(__LINE__, __FILE__,                             \
                               fn((x1), (x2), (x3), (x4), (x5), (x6)), #fn);   \
        }

//#endif

#endif