/*
 * Copyright (C) 2021 by Andreas Theofilu <andreas@theosys.at>
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

import android.content.ContentUris;
import android.content.Context;
import android.content.ContentResolver;
import android.database.Cursor;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.os.Environment;
import android.provider.DocumentsContract;
import android.provider.DocumentsContract.Path;
import android.provider.MediaStore;
import android.provider.OpenableColumns;
import android.util.Log;
import android.webkit.MimeTypeMap;
import java.io.File;
import java.io.FileNotFoundException;

public class UriToPath
{
    public static String getFileName(Uri uri, Context context)
    {
        Log.d("UriToPath", "Converting URI " + uri.toString() + " to a path ...");
        String fileName = getRealPathFromURI(context, uri);

        if (fileName == null)
        {
            fileName = "";
        }

        Log.d("UriToPath", "Found filename: " + fileName);
        return fileName;
    }

    /**
     * Get a file path from a Uri. This will get the the path for Storage Access
     * Framework Documents, as well as the _data field for the MediaStore and
     * other file-based ContentProviders.
     *
     * @param context The context.
     * @param uri     The Uri to query.
     * @author paulburke
     */
    public static String getRealPathFromURI(final Context context, final Uri uri)
    {
        final boolean isKitKat = Build.VERSION.SDK_INT >= Build.VERSION_CODES.KITKAT;

        // DocumentProvider
        if (isKitKat && DocumentsContract.isDocumentUri(context, uri))
        {
            Log.d("UriToPath", "Found document URI ...");
            // ExternalStorageProvider
            if (isExternalStorageDocument(uri))
            {
                final String docId = DocumentsContract.getDocumentId(uri);
                final String[] split = docId.split(":");
                final String type = split[0];
                String storageDefinition;

                // content://com.android.externalstorage.documents/document/primary%3ADocuments%2Ftpanel.log
                if ("primary".equalsIgnoreCase(type))
                    return Environment.getExternalStorageDirectory() + "/" + split[1];

                // content://com.android.externalstorage.documents/document/17ED-280E%3ADocuments%2Ftpanel.log
                if(Environment.isExternalStorageRemovable())
                    storageDefinition = "EXTERNAL_STORAGE";
                else
                {
                    storageDefinition = "SECONDARY_STORAGE";

                    if (System.getenv(storageDefinition) == null)
                        storageDefinition = "EXTERNAL_STORAGE";
                }

                Log.d("UriToPath", "Trying environment " + storageDefinition);
                return System.getenv(storageDefinition) + "/" + split[1];
            }
            // DownloadsProvider
            else if (isDownloadsDocument(uri))
            {
                final String id = DocumentsContract.getDocumentId(uri);
                final Uri contentUri = ContentUris.withAppendedId(
                        Uri.parse("content://downloads/public_downloads"), Long.valueOf(id));

                return getDataColumn(context, contentUri, null, null);
            }
            // MediaProvider
            else if (isMediaDocument(uri))
            {
                final String docId = DocumentsContract.getDocumentId(uri);
                final String[] split = docId.split(":");
                final String type = split[0];

                Uri contentUri = null;
                if ("image".equals(type))
                {
                    contentUri = MediaStore.Images.Media.EXTERNAL_CONTENT_URI;
                }
                else if ("video".equals(type))
                {
                    contentUri = MediaStore.Video.Media.EXTERNAL_CONTENT_URI;
                }
                else if ("audio".equals(type))
                {
                    contentUri = MediaStore.Audio.Media.EXTERNAL_CONTENT_URI;
                }

                final String selection = "_id=?";
                final String[] selectionArgs = new String[]
                {
                        split[1]
                };

                return getDataColumn(context, contentUri, selection, selectionArgs);
            }
        }
        else if (isKitKat && isExternalStorageDocument(uri))
        {
            Log.d("UriToPath", "Found external storage document ...");
            // ExternalStorageProvider
            final String docId = uri.getPath(); //DocumentsContract.getDocumentId(uri);

            if (docId == null)
                return null;

            final String[] split = docId.split(":");
            final String type = split[0];

            String fullPath = getPathFromExtSD(split);

            if (fullPath != "")
                return fullPath;
            else
                return null;
        }
        else if ("content".equalsIgnoreCase(uri.getScheme()))   // MediaStore (and general)
        {
            Log.d("UriToPath", "Found media store or general ...");
            // Return the remote address
            if (isGooglePhotosUri(uri))
                return uri.getLastPathSegment();
            else if (isPrimaryUri(uri))
            {
                try
                {
                    Log.d("UriToPath", "Trying to resolve a primary URI ...");
/*                    Uri u = uri;

                    if (!uri.getPath().contains("."))
                    {
                        String s = uri.toString() + "/x.txt";
                        u = uri.parse(s);
                        Log.d("UriToPath", "Using faked uri " + u.toString());
                    }
*/
                    ContentResolver resolver = context.getContentResolver();
                    Bundle metadata = DocumentsContract.getDocumentMetadata(resolver, uri);
                    return metadata.toString();
//                    DocumentsContract.Path path = DocumentsContract.findDocumentPath(resolver, u);
//                    return path.toString();
                }
                catch (FileNotFoundException ignore)
                {
                    Log.e("UriToPath", "Error: File not found!");
                    return null;
                }
                catch (IllegalArgumentException ignore)
                {
                    Log.e("UriToPath", "Uri \"" + uri.toString() + "\" contains an illegal argument!");
                    return null;
                }
                catch (NullPointerException ignore)
                {
                    Log.e("UriToPath", "Null pointer exception!");
                    return null;
                }
            }

            try
            {
                return getDataColumn(context, uri, null, null);
            }
            catch (IllegalArgumentException ign)
            {
                Log.e("UriToPath", "Uri \"" + uri.toString() + "\" contains an illegal argument!");
                return null;
            }
        }
        else if ("file".equalsIgnoreCase(uri.getScheme()))      // File
        {
            return uri.getPath();
        }

        return null;
    }

    private static boolean fileExists(String filePath)
    {
        File file = new File(filePath);

        return file.exists();
    }

    private static String getPathFromExtSD(String[] pathData)
    {
        final String type = pathData[0];
        final String relativePath = "/" + pathData[1];
        String fullPath = "";

        // on my Sony devices (4.4.4 & 5.1.1), `type` is a dynamic string
        // something like "71F8-2C0A", some kind of unique id per storage
        // don't know any API that can get the root path of that storage based on its id.
        //
        // so no "primary" type, but let the check here for other devices
        if ("primary".equalsIgnoreCase(type))
        {
            fullPath = Environment.getExternalStorageDirectory() + relativePath;

            if (fileExists(fullPath))
                return fullPath;
        }

        // Environment.isExternalStorageRemovable() is `true` for external and internal storage
        // so we cannot relay on it.
        //
        // instead, for each possible path, check if file exists
        // we'll start with secondary storage as this could be our (physically) removable sd card
        fullPath = System.getenv("SECONDARY_STORAGE") + relativePath;

        if (fileExists(fullPath))
            return fullPath;

        fullPath = System.getenv("EXTERNAL_STORAGE") + relativePath;

        if (fileExists(fullPath))
            return fullPath;

        return null;
    }

    /**
     * Get the value of the data column for this Uri. This is useful for
     * MediaStore Uris, and other file-based ContentProviders.
     *
     * @param context       The context.
     * @param uri           The Uri to query.
     * @param selection     (Optional) Filter used in the query.
     * @param selectionArgs (Optional) Selection arguments used in the query.
     * @return The value of the _data column, which is typically a file path.
     */
    public static String getDataColumn(Context context, Uri uri, String selection, String[] selectionArgs)
    {
        Log.d("UriToPath", "getDataColumn(Context context, Uri uri, String selection, String[] selectionArgs)");
        Cursor cursor = null;
        final String column = "_data";
        final String[] projection = { column };

        try
        {
            cursor = context.getContentResolver().query(uri, projection, selection, selectionArgs, null);

            if (cursor != null && cursor.moveToFirst())
            {
                final int index = cursor.getColumnIndexOrThrow(column);
                return cursor.getString(index);
            }
        }
        catch (RuntimeException ignore)
        {
            Log.e("UriToPath", "Runtime exception occured!");
            ignore.printStackTrace();
        }
        finally
        {
            if (cursor != null)
                cursor.close();
        }

        return null;
    }


    /**
     * @param uri The Uri to check.
     * @return Whether the Uri authority is ExternalStorageProvider.
     */
    public static boolean isExternalStorageDocument(Uri uri)
    {
        Log.d("UriToPath", "isExternalStorageDocument(Uri uri)");
        return "com.android.externalstorage.documents".equals(uri.getAuthority());
    }

    /**
     * @param uri The Uri to check.
     * @return Whether the Uri authority is DownloadsProvider.
     */
    public static boolean isDownloadsDocument(Uri uri)
    {
        Log.d("UriToPath", "isDownloadsDocument(Uri uri)");
        return "com.android.providers.downloads.documents".equals(uri.getAuthority());
    }

    /**
     * @param uri The Uri to check.
     * @return Whether the Uri authority is MediaProvider.
     */
    public static boolean isMediaDocument(Uri uri)
    {
        Log.d("UriToPath", "isMediaDocument(Uri uri)");
        return "com.android.providers.media.documents".equals(uri.getAuthority());
    }

    /**
     * @param uri The Uri to check.
     * @return Whether the Uri authority is Google Photos.
     */
    public static boolean isGooglePhotosUri(Uri uri)
    {
        Log.d("UriToPath", "isGooglePhotosUri(Uri uri)");
        return "com.google.android.apps.photos.content".equals(uri.getAuthority());
    }

    public static boolean isPrimaryUri(Uri uri)
    {
        Log.d("UriToPath", "isPrimaryUri(Uri uri) --> " + uri.getPath());
        return uri.getPath().contains("tree/primary");
    }
}
