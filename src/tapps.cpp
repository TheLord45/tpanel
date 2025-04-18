/*
 * Copyright (C) 2024 by Andreas Theofilu <andreas@theosys.at>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
 */

#include "tapps.h"
#include "ttpinit.h"
#include "texpat++.h"
#include "terror.h"

#if __cplusplus < 201402L
#   error "This module requires at least C++14 standard!"
#else
#   if __cplusplus < 201703L
#       include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#       warning "Support for C++14 and experimental filesystem will be removed in a future version!"
#   else
#       include <filesystem>
#       ifdef __ANDROID__
namespace fs = std::__fs::filesystem;
#       else
namespace fs = std::filesystem;
#       endif
#   endif
#endif

using std::string;
using std::vector;
using namespace Expat;

vector<APP_SETTINGS_t> TApps::mAppSettings;
vector<APP_WINDOW_FRAME_t> TApps::mWindowFrames;

TApps::TApps()
{
    DECL_TRACER("TApps::TApps()");
}

TApps::~TApps()
{
    DECL_TRACER("TApps::~TApps()");
}

bool TApps::parseApps()
{
    DECL_TRACER("TApps::parseApps()");

    if (!TTPInit::getTP5())
    {
        MSG_WARNING("Can't read app configs because it's not a TP5 format!");
        return false;
    }

    if (!mAppSettings.empty())
        mAppSettings.clear();

    string projectPath = TConfig::getProjectPath();

    if (!fs::exists(projectPath + "/G5Apps.xma"))
        return false;

    string path = makeFileName(projectPath, "G5Apps.xma");

    if (!isValidFile())
    {
        MSG_ERROR("File " << path << " doesn't exist or is not readable!");
        TError::setError();
        return false;
    }

    TExpat xml(path);
    xml.setEncoding(ENC_UTF8);

    if (!xml.parse())
        return false;

    int depth = 0;
    size_t index = 0;

    if ((index = xml.getElementIndex("Apps", &depth)) == TExpat::npos)
    {
        MSG_DEBUG("File does not contain the element \"Apps\"!");
        TError::setError();
        return false;
    }

    depth++;
    string name, content;
    vector<ATTRIBUTE_t> attrs;

    while ((index = xml.getNextElementFromIndex(index, &name, &content, &attrs)) != TExpat::npos)
    {
        if (name.compare("App") == 0)
        {
            APP_SETTINGS_t app;

            app.appName = xml.getAttribute("name", attrs);
            app.packageName = xml.getAttribute("packageName", attrs);
            app.appID = xml.getAttribute("appID", attrs);

            string a;

            while ((index = xml.getNextElementFromIndex(index, &a, &content, &attrs)) != TExpat::npos)
            {
                if (a.compare("Info") == 0)
                    app.appInfo = content;
                else if (a.compare("Window") == 0)
                {
                    app.appWindow.aspectFixed = xml.getAttributeBool("aspectFixed", attrs);
                    string s;

                    while ((index = xml.getNextElementFromIndex(index, &s, &content, &attrs)) != TExpat::npos)
                    {
                        if (s.compare("AspectRatios") == 0)
                        {
                            APP_ASPECT_RATIO_t ar;
                            string r;

                            while ((index = xml.getNextElementFromIndex(index, &r, &content, &attrs)) != TExpat::npos)
                            {
                                if (r.compare("AspectRatio") == 0)
                                {
                                    ar.percent = xml.getAttributeDouble("percent", attrs);
                                    ar.ratioWidth = xml.getAttributeInt("ratioWidth", attrs);
                                    ar.ratioHeight = xml.getAttributeInt("ratioHeight", attrs);
                                    app.appWindow.aspectRatios.push_back(ar);
                                }
                            }
                        }
                        else if (s.compare("AspectRatioLimits") == 0)
                        {
                            app.appWindow.aspectRatioLimits.minWidth = xml.getAttributeInt("minWidth", attrs);
                            app.appWindow.aspectRatioLimits.minHeight = xml.getAttributeInt("minHeight", attrs);
                        }
                    }
                }
                else if (a.compare("Images") == 0)
                {
                    string i;

                    while ((index = xml.getNextElementFromIndex(index, &i, &content, &attrs)) != TExpat::npos)
                    {
                        if (i.compare("ThumbImage") == 0)
                            app.appImages.thumbImage = content;
                        else if (i.compare("WindowImage") == 0)
                            app.appImages.windowImage = content;
                    }
                }
                else if (name.compare("Parameters") == 0)
                {
                    string p;
                    APP_PARAMETER_t par;

                    while ((index = xml.getNextElementFromIndex(index, &p, &content, &attrs)) != TExpat::npos)
                    {
                        if (p.compare("Parameter") == 0)
                        {
                            par.name = xml.getAttribute("name", attrs);
                            par.eDataType = xml.getAttribute("eDataType", attrs);
                            par.value = xml.getAttribute("value", attrs);
                            par.info = xml.getAttribute("info", attrs);
                            APP_PAR_STRINGS_t val;
                            string v;

                            while ((index = xml.getNextElementFromIndex(index, &v, &content, &attrs)) != TExpat::npos)
                            {
                                if (v.compare("StringValues") == 0)
                                {
                                    string sv;

                                    while ((index = xml.getNextElementFromIndex(index, &sv, &content, &attrs)) != TExpat::npos)
                                    {
                                        if (sv.compare("StringValue") == 0)
                                        {
                                            val.key = xml.getAttribute("key", attrs);
                                            val.value = content;
                                            par.stringValues.push_back(val);
                                        }
                                    }
                                }
                            }
                        }
                    }

                    app.parameters.push_back(par);
                }
            }

            mAppSettings.push_back(app);
        }
        else if (name.compare("WindowFrames") == 0)
        {
            string w;
            APP_WINDOW_FRAME_t wf;

            while ((index = xml.getNextElementFromIndex(index, &w, &content, &attrs)) != TExpat::npos)
            {
                if (w.compare("WindowFrame") == 0)
                {
                    wf.eType = xml.getAttribute("eType", attrs);
                    wf.edgeSize = xml.getAttributeInt("edgeSize", attrs);
                    wf.barSize = xml.getAttributeInt("barSize", attrs);
                    string b;

                    while ((index = xml.getNextElementFromIndex(index, &b, &content, &attrs)) != TExpat::npos)
                    {
                        APP_BUTTON_t button;

                        if (b.compare("Buttons") == 0)
                        {
                            string a;

                            while ((index = xml.getNextElementFromIndex(index, &a, &content, &attrs)) != TExpat::npos)
                            {
                                if (a.compare("Button") == 0)
                                {
                                    button.eLocation = xml.getAttribute("eLocation", attrs);
                                    button.order = xml.getAttributeInt("order", attrs);
                                    button.spacing = xml.getAttributeInt("spacing", attrs);
                                    string i;

                                    while ((index = xml.getNextElementFromIndex(index, &i, &content, &attrs)) != TExpat::npos)
                                    {
                                        if (i.compare("ButtonImage") == 0)
                                            button.buttonImage.push_back(content);
                                    }
                                }
                            }

                            wf.buttons.push_back(button);
                        }
                    }

                    mWindowFrames.push_back(wf);
                }
            }
        }
    }

    return true;
}

APP_SETTINGS_t TApps::getApp(const string& name)
{
    DECL_TRACER("TApps::getApp(const string& name)");

    if (mAppSettings.empty())
        return APP_SETTINGS_t();

    vector<APP_SETTINGS_t>::iterator iter;

    for (iter = mAppSettings.begin(); iter != mAppSettings.end(); ++iter)
    {
        if (iter->appID == name)
            return *iter;
    }

    return APP_SETTINGS_t();
}

APP_WINDOW_FRAME_t TApps::getWindowFrame(const string& type)
{
    DECL_TRACER("TApps::getWindowFrame(const string& type)");

    if (mWindowFrames.empty())
        return APP_WINDOW_FRAME_t();

    vector<APP_WINDOW_FRAME_t>::iterator iter;

    for (iter = mWindowFrames.begin(); iter != mWindowFrames.end(); ++iter)
    {
        if (iter->eType == type)
            return *iter;
    }

    return APP_WINDOW_FRAME_t();
}
