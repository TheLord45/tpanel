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

#ifndef __TSYSTEMSOUND_H__
#define __TSYSTEMSOUND_H__

#include <string>
#include <vector>

class TSystemSound
{
    public:
        TSystemSound(const std::string& path);
        ~TSystemSound();

        /**
         * Determines the currently set sound for touch feedback and returns
         * the path and name of the soundfile to play.
         *
         * The precondition for this function is a prevously set path to the
         * directory containing the system sounds.
         *
         * @return On success the path and name of the sound file is returned.
         * Otherwise an empty string is returned.
         */
        std::string getTouchFeedbackSound();
        /**
         * Determines whether the system sounds are activated or not and
         * returns the result.
         *
         * The precondition for this function is a prevously set path to the
         * directory containing the system sounds.
         *
         * @return If the system sounds are activated, it returns TRUE.
         * If there is no valid path to the system sounds set, then it returns
         * FALSE.
         */
        bool getSystemSoundState();

        std::string getFirstSingleBeep();
        std::string getNextSingleBeep();
        std::string getFirstDoubleBeep();
        std::string getNextDoubleBeep();
        std::string& getTestSound() { return mTestSound; }
        std::string& getDockSound() { return mDocked; }
        std::string& getRingBackSound() { return mRingBack; }
        std::string& getRingToneSound() { return mRingTone; }

        void setPath(const std::string& path);
        void setFile(const std::string& file);

    protected:
        bool readAllSystemSounds();

    private:
        std::string mPath;      // The path to the system sounds
        std::string mFile;      // File name of touch sound file
        bool mValid{false};     // TRUE = valid path was set
        std::vector<std::string> mSinglePeeps;
        std::vector<std::string> mDoubleBeeps;
        std::string mTestSound;
        std::string mDocked;
        std::string mRingBack;
        std::string mRingTone;
        size_t mSinglePos{0};
        size_t mDoublePos{0};
};

#endif
