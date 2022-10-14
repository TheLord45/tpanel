/*
 * Copyright (C) 2020, 2021 by Andreas Theofilu <andreas@theosys.at>
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

#include "tsystemsound.h"
#include "tvalidatefile.h"
#include "tconfig.h"
#include "terror.h"
#include "tresources.h"

#include <algorithm>

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

static std::vector<std::string> mAllSounds;    // Cache

TSystemSound::TSystemSound(const string& path)
        : mPath(path)
{
    DECL_TRACER("TSystemSound::TSystemSound(const string& path)");

    TValidateFile vf;

    if (vf.isValidDir(path))
        mValid = true;
    else
    {
        MSG_WARNING("The path " << path << " is invalid!");
    }

    mFile = TConfig::getSystemSound();
    string p = path + "/" + mFile;

    if (mFile.empty() || !vf.isValidFile(p))
    {
        MSG_WARNING("No or invalid file (" << p << ")");
        mValid = false;
    }

    if (mValid)
        readAllSystemSounds();
}

TSystemSound::~TSystemSound()
{
    DECL_TRACER("TSystemSound::~TSystemSound()");
}

string TSystemSound::getTouchFeedbackSound()
{
    DECL_TRACER("TSystemSound::getTouchFeedbackSound()");

    if (!mValid)
        return string();

    return mPath + "/" + mFile;
}

bool TSystemSound::getSystemSoundState()
{
    DECL_TRACER("TSystemSound::getSystemSoundState()");

    if (!mValid)
    {
        MSG_ERROR("No or invalid path!");
        return false;
    }

    return TConfig::getSystemSoundState();
}

void TSystemSound::setPath(const string& path)
{
    DECL_TRACER("TSystemSound::setPath(const string& path)");

    TValidateFile vf;

    if (vf.isValidDir(path))
    {
        mPath = path;
        mValid = true;
    }
    else
    {
        MSG_WARNING("Path " << path << " is invalid and was ignored!");
    }
}

void TSystemSound::setFile(const string& file)
{
    DECL_TRACER("TSystemSound::setFile(const string& file)");

    if (!mValid)
    {
        MSG_WARNING("Because of no or an invalid path the file " << file << " will be ignored!");
        return;
    }

    string p = mPath + "/" + file;
    TValidateFile vf;

    if (!vf.isValidFile(p))
    {
        MSG_WARNING("The file " << file << " doesn't exist!");
        return;
    }

    mFile = file;
}

bool TSystemSound::readAllSystemSounds()
{
    DECL_TRACER("TSystemSound::readAllSystemSounds()");

    if (!mValid)
        return false;

    if (mAllSounds.empty())
    {
        try
        {
            for(auto& p: fs::directory_iterator(mPath))
            {
                string f = fs::path(p.path()).filename();

                if (f.at(0) == '.' || fs::is_directory(p.path()))
                    continue;

                if (fs::is_regular_file(p.path()))
                {
                    MSG_DEBUG("Found sound file " << f);
                    mAllSounds.push_back(f);

                    if (startsWith(f, "singleBeep"))
                        mSinglePeeps.push_back(f);
                    else if (startsWith(f, "doubleBeep"))
                        mDoubleBeeps.push_back(f);
                    else if (startsWith(f, "audio"))
                        mTestSound = f;
                    else if (startsWith(f, "docked"))
                        mDocked = f;
                    else if (startsWith(f, "ringback"))
                        mRingBack = f;
                    else if (startsWith(f, "ringtone"))
                        mRingTone = f;
                }
            }
        }
        catch(std::exception& e)
        {
            MSG_ERROR("Error: " << e.what());
            return false;
        }
    }
    else
        filterSounds();

    if (mSinglePeeps.size() > 0)
        std::sort(mSinglePeeps.begin(), mSinglePeeps.end());

    if (mDoubleBeeps.size() > 0)
        std::sort(mDoubleBeeps.begin(), mDoubleBeeps.end());

    return true;
}

void TSystemSound::filterSounds()
{
    DECL_TRACER("TSystemSound::filterSounds()");

    if (mAllSounds.empty())
        return;

    vector<string>::iterator iter;

    for (iter = mAllSounds.begin(); iter != mAllSounds.end(); ++iter)
    {
        string f = *iter;

        if (startsWith(f, "singleBeep"))
            mSinglePeeps.push_back(f);
        else if (startsWith(f, "doubleBeep"))
            mDoubleBeeps.push_back(f);
        else if (startsWith(f, "audio"))
            mTestSound = f;
        else if (startsWith(f, "docked"))
            mDocked = f;
        else if (startsWith(f, "ringback"))
            mRingBack = f;
        else if (startsWith(f, "ringtone"))
            mRingTone = f;
    }
}

string TSystemSound::getFirstSingleBeep()
{
    DECL_TRACER("TSystemSound::getFirstSingleBeep()");
    mSinglePos = 0;

    if (mSinglePeeps.size() == 0)
        return string();

    mSinglePos++;
    return mSinglePeeps.at(0);
}

string TSystemSound::getNextSingleBeep()
{
    DECL_TRACER("TSystemSound::getNextSingleBeep()");

    if (mSinglePeeps.size() >= mSinglePos)
        return string();

    size_t old = mSinglePos;
    mSinglePos++;
    return mSinglePeeps.at(old);
}

string TSystemSound::getFirstDoubleBeep()
{
    DECL_TRACER("TSystemSound::getFirstDoubleBeep()");
    mDoublePos = 0;

    if (mDoubleBeeps.size() == 0)
        return string();

    mDoublePos++;
    return mDoubleBeeps.at(0);
}

string TSystemSound::getNextDoubleBeep()
{
    DECL_TRACER("TSystemSound::getNextDoubleBeep()");

    if (mDoubleBeeps.size() >= mDoublePos)
        return string();

    size_t old = mDoublePos;
    mDoublePos++;
    return mDoubleBeeps.at(old);
}
