/*
 * Copyright (C) 2025 by Andreas Theofilu <andreas@theosys.at>
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

package org.qtproject.theosys;

import android.media.MediaPlayer;
import android.media.AudioAttributes;

import org.qtproject.theosys.Logger;

public class PlaySound extends Logger
{
    static private MediaPlayer mediaPlayer = null;
    static private boolean bBlocked = false;
    static private String lastSource;
    static private boolean muted = false;
    static private float oldVolume = 0;

    static public void play(String file, float volume)
    {
        log(HLOG_TRACE, "PlaySound->play(String uri)");

        if (mediaPlayer != null && mediaPlayer.isPlaying())
            mediaPlayer.stop();

        if (volume == 0.0)
        {
            muted = true;
            return;
        }
        else
            muted = false;

        bBlocked = true;

        try
        {
            if (mediaPlayer == null)
            {
                log(HLOG_DEBUG, "Creating new media player ...");

                mediaPlayer = new MediaPlayer();
                mediaPlayer.setAudioAttributes(
                    new AudioAttributes.Builder()
                        .setContentType(AudioAttributes.CONTENT_TYPE_MUSIC)
                        .setUsage(AudioAttributes.USAGE_MEDIA)
                        .build()
                );

                mediaPlayer.setOnCompletionListener(new MediaPlayer.OnCompletionListener() {
                    public void onCompletion(MediaPlayer mp) {
                        mediaPlayer.release();
                        mediaPlayer = null;
                        log(HLOG_INFO, "Sound file completed/stopped playing.");
                    }
                });

                mediaPlayer.setOnInfoListener(new MediaPlayer.OnInfoListener() {
                    public boolean onInfo(MediaPlayer mp, int what, int extra) {
                        switch(what)
                        {
                            case MediaPlayer.MEDIA_INFO_BUFFERING_START:    log(HLOG_INFO, "Buffering started ..."); break;
                            case MediaPlayer.MEDIA_INFO_BUFFERING_END:      log(HLOG_INFO, "Buffering completed."); break;
                            case MediaPlayer.MEDIA_INFO_BAD_INTERLEAVING:   log(HLOG_INFO, "Bad interleaving!"); break;
                            case MediaPlayer.MEDIA_INFO_NOT_SEEKABLE:       log(HLOG_INFO, "Track is not seekable."); break;
                            case MediaPlayer.MEDIA_INFO_METADATA_UPDATE:    log(HLOG_INFO, "Matadata updated."); break;
                            default:
                                log(HLOG_WARNING, "Unknown information " + String.valueOf(what) + " found!");
                                return false;
                        }

                        return true;
                    }
                });
            }

            if (file != lastSource)
            {
                log(HLOG_DEBUG, "Setting new source: " + file);
                mediaPlayer.setDataSource(file);
                lastSource = file;
            }

            if (volume >= 0.0 && volume <= 1.0)
            {
                log(HLOG_DEBUG, "Setting volume to: " + String.valueOf(volume * 100.0));
                mediaPlayer.setVolume(volume, volume);
                oldVolume = volume;
            }
            else
                log(HLOG_WARNING, "Invalid volume " + String.valueOf(volume) + ". Ignoring it!");

            log(HLOG_DEBUG, "Preparing source ...");
            mediaPlayer.prepare();
            log(HLOG_INFO, "Start to play sound ...");
            mediaPlayer.start();
        }
        catch (Exception e)
        {
            log(HLOG_ERROR, "PlaySound->play: Exception: " + e);
        }

        bBlocked = false;
    }

    static public void stop()
    {
        log(HLOG_TRACE, "PlaySound->stop()");

        if (mediaPlayer != null && mediaPlayer.isPlaying())
            mediaPlayer.stop();
    }

    static public void setMute(boolean mute)
    {
        log(HLOG_TRACE, "PlaySound->setMute(boolean mute)");
        if (mute == muted)
            return;

        muted = mute;

        if (mediaPlayer != null)
        {
            if (muted && mediaPlayer.isPlaying())
                mediaPlayer.setVolume(0, 0);
            else if (mediaPlayer.isPlaying())
                mediaPlayer.setVolume(oldVolume, oldVolume);
        }
    }

    static public void setVolume(float volume)
    {
        log(HLOG_TRACE, "PlaySound->setVolume(float volume)");

        if (volume == oldVolume)
            return;

        if (mediaPlayer != null && volume >= 0.0 && volume <= 1.0)
        {
            mediaPlayer.setVolume(volume, volume);
            oldVolume = volume;
        }
    }
}
