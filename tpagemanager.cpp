/*
 * Copyright (C) 2020 to 2022 by Andreas Theofilu <andreas@theosys.at>
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

#include <vector>
#include <thread>
#include <mutex>
#ifdef __ANDROID__
#include <QtAndroidExtras/QAndroidJniObject>
#include <QtAndroid>
#include <android/log.h>
#endif

#include "tpagemanager.h"
#include "tcolor.h"
#include "terror.h"
#include "ticons.h"
#include "tbutton.h"
#include "tprjresources.h"
#include "tresources.h"
#include "tobject.h"
#include "tsystemsound.h"
#include "tvalidatefile.h"
#include "ttpinit.h"

using std::vector;
using std::string;
using std::map;
using std::pair;
using std::to_string;
using std::thread;
using std::atomic;
using std::mutex;
using std::bind;

TIcons *gIcons = nullptr;
TPrjResources *gPrjResources = nullptr;
TPageManager *gPageManager = nullptr;
extern amx::TAmxNet *gAmxNet;
extern std::atomic<bool> _netRunning;
extern bool _restart_;                          //!< If this is set to true then the whole program will start over.

mutex surface_mutex;
bool prg_stopped = false;

#ifdef __ANDROID__
JNIEXPORT void JNICALL Java_org_qtproject_theosys_BatteryState_informBatteryStatus(JNIEnv *, jclass, jint level, jboolean charging, jint chargeType)
{
    DECL_TRACER("JNICALL Java_org_qtproject_theosys_BatteryState_informBatteryStatus(JNIEnv *, jclass, jint level, jboolean charging, jint chargeType)");

    if (gPageManager)
        gPageManager->informBatteryStatus(level, charging, chargeType);
}

JNIEXPORT void JNICALL Java_org_qtproject_theosys_NetworkStatus_informTPanelNetwork(JNIEnv */*env*/, jclass /*clazz*/, jboolean conn, jint level, jint type)
{
    DECL_TRACER("JNICALL Java_org_qtproject_theosys_NetworkStatus_informTPanelNetwork(JNIEnv */*env*/, jclass /*clazz*/, jboolean conn, jint level, jint type)");
   //Call native side conterpart
   if (gPageManager)
        gPageManager->informTPanelNetwork(conn, level, type);
}

JNIEXPORT void JNICALL Java_org_qtproject_theosys_PhoneCallState_informPhoneState(JNIEnv *env, jclass, jboolean call, jstring pnumber)
{
    DECL_TRACER("JNICALL Java_org_qtproject_theosys_PhoneCallState_informPhoneState(JNIEnv *env, jclass, jboolean call, jstring pnumber)");

    string phoneNumber;

    if (pnumber)
    {
        const jclass stringClass = env->GetObjectClass(pnumber);
        const jmethodID getBytes = env->GetMethodID(stringClass, "getBytes", "(Ljava/lang/String;)[B");
        const jbyteArray stringJbytes = (jbyteArray) env->CallObjectMethod(pnumber, getBytes, env->NewStringUTF("UTF-8"));

        size_t length = (size_t) env->GetArrayLength(stringJbytes);
        jbyte* pBytes = env->GetByteArrayElements(stringJbytes, NULL);

        phoneNumber = string((char *)pBytes, length);
        env->ReleaseByteArrayElements(stringJbytes, pBytes, JNI_ABORT);

        env->DeleteLocalRef(stringJbytes);
        env->DeleteLocalRef(stringClass);
    }

    if (gPageManager)
        gPageManager->informPhoneState(call, phoneNumber);
}

JNIEXPORT void JNICALL Java_org_qtproject_theosys_Logger_logger(JNIEnv *env, jclass, jint mode, jstring msg)
{
    DECL_TRACER("JNICALL Java_org_qtproject_theosys_Logger_logger(JNIEnv *env, jclass cl, jint mode, jstring msg)");

    if (!msg)
        return;

    const jclass stringClass = env->GetObjectClass(msg);
    const jmethodID getBytes = env->GetMethodID(stringClass, "getBytes", "(Ljava/lang/String;)[B");
    const jbyteArray stringJbytes = (jbyteArray) env->CallObjectMethod(msg, getBytes, env->NewStringUTF("UTF-8"));

    size_t length = (size_t) env->GetArrayLength(stringJbytes);
    jbyte* pBytes = env->GetByteArrayElements(stringJbytes, NULL);

    string ret = std::string((char *)pBytes, length);
    env->ReleaseByteArrayElements(stringJbytes, pBytes, JNI_ABORT);

    env->DeleteLocalRef(stringJbytes);
    env->DeleteLocalRef(stringClass);

    try
    {
        TError::lock();

        if (TStreamError::checkFilter(mode))
            TError::Current()->logMsg(TError::append(mode, *TStreamError::getStream()) << ret << std::endl);

        TError::unlock();
    }
    catch (std::exception& e)
    {
        __android_log_print(ANDROID_LOG_ERROR, "tpanel", "%s", e.what());
    }
}

JNIEXPORT void JNICALL Java_org_qtproject_theosys_Orientation_informTPanelOrientation(JNIEnv */*env*/, jclass /*clazz*/, jint orientation)
{
    DECL_TRACER("Java_org_qtproject_theosys_Orientation_informTPanelOrientation(JNIEnv */*env*/, jclass /*clazz*/, jint orientation)");

    MSG_PROTOCOL("Received android orientation " << orientation);

    if (gPageManager && gPageManager->onOrientationChange())
        gPageManager->onOrientationChange()(orientation);
}
#endif

TPageManager::TPageManager()
{
    surface_mutex.lock();
    DECL_TRACER("TPageManager::TPageManager()");

    gPageManager = this;
    TTPInit *tinit = new TTPInit;
    tinit->setPath(TConfig::getProjectPath());

    if (tinit->isVirgin())
        tinit->loadSurfaceFromController();

    delete tinit;
    // Read the AMX panel settings.
    mTSettings = new TSettings(TConfig::getProjectPath());

    if (TError::isError())
    {
        MSG_ERROR("Settings were not read successfull!");
        surface_mutex.unlock();
        return;
    }

    readMap();  // Start the initialisation of the AMX part.

    gPrjResources = new TPrjResources(mTSettings->getResourcesList());
    mPalette = new TPalette();
    vector<PALETTE_SETUP> pal = mTSettings->getSettings().palettes;

    if (pal.size() > 0)
    {
        vector<PALETTE_SETUP>::iterator iterPal;

        for (iterPal = pal.begin(); iterPal != pal.end(); ++iterPal)
            mPalette->initialize(iterPal->file);
    }

    if (!TError::isError())
        TColor::setPalette(mPalette);

    mFonts = new TFont();

    if (TError::isError())
    {
        MSG_ERROR("Initializing fonts was not successfull!");
    }

    gIcons = new TIcons();

    if (TError::isError())
    {
        MSG_ERROR("Initializing icons was not successfull!");
    }

    mPageList = new TPageList();
    mExternal = new TExternal();
    PAGELIST_T page;

    if (!mTSettings->getSettings().powerUpPage.empty())
    {
        if (readPage(mTSettings->getSettings().powerUpPage))
        {
            MSG_TRACE("Found power up page " << mTSettings->getSettings().powerUpPage);
            page = findPage(mTSettings->getSettings().powerUpPage);
            mActualPage = page.pageID;
        }
    }
    else
    {
        MSG_WARNING("No power up page defined! Setting default page to 1.");
        mActualPage = 1;
    }

    TPage *pg = getPage(mActualPage);

    vector<string> popups = mTSettings->getSettings().powerUpPopup;

    if (popups.size() > 0)
    {
        vector<string>::iterator iter;

        for (iter = popups.begin(); iter != popups.end(); ++iter)
        {
            if (readSubPage(*iter))
            {
                MSG_TRACE("Found power up popup " << *iter);

                if (pg)
                {
                    TSubPage *spage = getSubPage(*iter);
                    pg->addSubPage(spage);
                }
            }
        }
    }

    // Here we initialize the system resources like borders, cursors, sliders, ...
    mSystemDraw = new TSystemDraw(TConfig::getSystemPath(TConfig::BASE));

    // Here are the commands supported by this emulation.
    MSG_INFO("Registering commands ...");
//    REG_CMD(doAFP, "@AFP");     // Flips to a page with the specified page name using an animated transition.
//    REG_CMD(doAFP, "^AFP");     // Flips to a page with the specified page name using an animated transition.
    REG_CMD(doAPG, "@APG");     // Add a specific popup page to a specified popup group.
    REG_CMD(doCPG, "@CPG");     // Clear all popup pages from specified popup group.
    REG_CMD(doDPG, "@DPG");     // Delete a specific popup page from specified popup group if it exists
//    REG_CMD(doPDR, "@PDR");     // Set the popup location reset flag.
    REG_CMD(doPHE, "@PHE");     // Set the hide effect for the specified popup page to the named hide effect.
    REG_CMD(doPHP, "@PHP");     // Set the hide effect position.
    REG_CMD(doPHT, "@PHT");     // Set the hide effect time for the specified popup page.
    REG_CMD(doPPA, "@PPA");     // Close all popups on a specified page.
    REG_CMD(doPPA, "^PPA");     // G5: Close all popups on a specified page.
    REG_CMD(doPPF, "@PPF");     // Deactivate a specific popup page on either a specified page or the current page.
    REG_CMD(doPPF, "^PPF");     // G5: Deactivate a specific popup page on either a specified page or the current page.
    REG_CMD(doPPF, "PPOF");     // Deactivate a specific popup page on either a specified page or the current page
    REG_CMD(doPPG, "@PPG");     // Toggle a specific popup page on either a specified page or the current page.
    REG_CMD(doPPG, "^PPG");     // G5: Toggle a specific popup page on either a specified page or the current page.
    REG_CMD(doPPG, "PPOG");     // Toggle a specific popup page on either a specified page or the current page.
    REG_CMD(doPPK, "@PPK");     // Kill a specific popup page from all pages.
    REG_CMD(doPPK, "^PPK");     // G5: Kill a specific popup page from all pages.
    REG_CMD(doPPM, "@PPM");     // Set the modality of a specific popup page to Modal or NonModal.
    REG_CMD(doPPM, "^PPM");     // G5: Set the modality of a specific popup page to Modal or NonModal.
    REG_CMD(doPPN, "@PPN");     // Activate a specific popup page to launch on either a specified page or the current page.
    REG_CMD(doPPN, "^PPN");     // G5: Activate a specific popup page to launch on either a specified page or the current page.
    REG_CMD(doPPN, "PPON");     // Activate a specific popup page to launch on either a specified page or the current page.
    REG_CMD(doPPT, "@PPT");     // Set a specific popup page to timeout within a specified time.
    REG_CMD(doPPT, "^PPT");     // G5: Set a specific popup page to timeout within a specified time.
    REG_CMD(doPPX, "@PPX");     // Close all popups on all pages.
    REG_CMD(doPPX, "^PPX");     // G5: Close all popups on all pages.
    REG_CMD(doPSE, "@PSE");     // Set the show effect for the specified popup page to the named show effect.
    REG_CMD(doPSP, "@PSP");     // Set the show effect position.
    REG_CMD(doPST, "@PST");     // Set the show effect time for the specified popup page.
    REG_CMD(doPAGE, "PAGE");    // Flip to a specified page.
    REG_CMD(doPAGE, "^PGE");    // G5: Flip to a specified page.

    REG_CMD(doANI, "^ANI");     // Run a button animation (in 1/10 second).
    REG_CMD(doAPF, "^APF");     // Add page flip action to a button if it does not already exist.
    REG_CMD(doBAT, "^BAT");     // Append non-unicode text.
    REG_CMD(doBAU, "^BAU");     // Append unicode text. Same format as ^UNI.
    REG_CMD(doBCB, "^BCB");     // Set the border color.
    REG_CMD(getBCB, "?BCB");    // Get the current border color.
    REG_CMD(doBCF, "^BCF");     // Set the fill color to the specified color.
    REG_CMD(getBCF, "?BCF");    // Get the current fill color.
    REG_CMD(doBCT, "^BCT");     // Set the text color to the specified color.
    REG_CMD(getBCT, "?BCT");    // Get the current text color.
    REG_CMD(doBDO, "^BDO");     // Set the button draw order
    REG_CMD(doBFB, "^BFB");     // Set the feedback type of the button.
    REG_CMD(doBMC, "^BMC");    // Button copy command.
//    REG_CMD(doBMI, "^BMI");    // Set the button mask image.
    REG_CMD(doBML, "^BML");    // Set the maximum length of the text area button.
    REG_CMD(doBMP, "^BMP");     // Assign a picture to those buttons with a defined addressrange.
    REG_CMD(getBMP, "?BMP");    // Get the current bitmap name.
    REG_CMD(doBOP, "^BOP");     // Set the button opacity.
    REG_CMD(getBOP, "?BOP");    // Get the button opacity.
    REG_CMD(doBOR, "^BOR");     // Set a border to a specific border style.
    REG_CMD(doBOS, "^BOS");     // Set the button to display either a Video or Non-Video window.
    REG_CMD(doBRD, "^BRD");     // Set the border of a button state/states.
    REG_CMD(getBRD, "?BRD");    // Get the border of a button state/states.
//    REG_CMD(doBSF, "^BSF");     // Set the focus to the text area.
    REG_CMD(doBSP, "^BSP");     // Set the button size and position.
    REG_CMD(doBSM, "^BSM");     // Submit text for text area buttons.
    REG_CMD(doBSO, "^BSO");     // Set the sound played when a button is pressed.
    REG_CMD(doBWW, "^BWW");     // Set the button word wrap feature to those buttons with a defined address range.
    REG_CMD(getBWW, "?BWW");    // Get the button word wrap feature to those buttons with a defined address range.
    REG_CMD(doCPF, "^CPF");     // Clear all page flips from a button.
    REG_CMD(doDPF, "^DPF");     // Delete page flips from button if it already exists.
    REG_CMD(doENA, "^ENA");     // Enable or disable buttons with a set variable text range.
    REG_CMD(doFON, "^FON");     // Set a font to a specific Font ID value for those buttons with a range.
    REG_CMD(getFON, "?FON");    // Get the current font index.
//    REG_CMD(doGDI, "^GDI");     // Change the bargraph drag increment.
//    REG_CMD(doGDV, "^GDV");     // Invert the joystick axis to move the origin to another corner.
    REG_CMD(doGLH, "^GLH");     // Change the bargraph upper limit.
    REG_CMD(doGLL, "^GLL");     // Change the bargraph lower limit.
    REG_CMD(doGSC, "^GSC");     // Change the bargraph slider color or joystick cursor color.
    REG_CMD(doICO, "^ICO");     // Set the icon to a button.
    REG_CMD(getICO, "?ICO");    // Get the current icon index.
    REG_CMD(doJSB, "^JSB");     // Set bitmap/picture alignment using a numeric keypad layout for those buttons with a defined address range.
    REG_CMD(getJSB, "?JSB");    // Get the current bitmap justification.
    REG_CMD(doJSI, "^JSI");     // Set icon alignment using a numeric keypad layout for those buttons with a defined address range.
    REG_CMD(getJSI, "?JSI");    // Get the current icon justification.
    REG_CMD(doJST, "^JST");     // Set text alignment using a numeric keypad layout for those buttons with a defined address range.
    REG_CMD(getJST, "?JST");    // Get the current text justification.
    REG_CMD(doSHO, "^SHO");     // Show or hide a button with a set variable text range.
    REG_CMD(doTEC, "^TEC");     // Set the text effect color for the specified addresses/states to the specified color.
    REG_CMD(getTEC, "?TEC");    // Get the current text effect color.
    REG_CMD(doTEF, "^TEF");     // Set the text effect. The Text Effect is specified by name and can be found in TPD4.
    REG_CMD(getTEF, "?TEF");    // Get the current text effect name.
//    REG_CMD(doTOP, "^TOP");     // Send events to the Master as string events.
    REG_CMD(doTXT, "^TXT");     // Assign a text string to those buttons with a defined address range.
    REG_CMD(getTXT, "?TXT");    // Get the current text information.
    REG_CMD(doUNI, "^UNI");     // Set Unicode text.
    REG_CMD(doUTF, "^UTF");     // G5: Set button state text using UTF-8 text command.

//    REG_CMD(doLPC, "^LPC");     // Clear all users from the User Access Passwords list on the Password Setup page.
//    REG_CMD(doLPR, "^LPR");     // Remove a given user from the User Access Passwords list on the Password Setup page.
//    REG_CMD(doLPS, "^LPS");     // Set the user name and password.

    REG_CMD(doKPS, "^KPS");     // Set the keyboard passthru.
    REG_CMD(doVKS, "^VKS");     // Send one or more virtual key strokes to the G4 application.

//    REG_CMD(doPWD, "@PWD");     // Set the page flip password.
//    REG_CMD(doPWD, "^PWD");     // Set the page flip password.

    REG_CMD(doBBR, "^BBR");     // Set the bitmap of a button to use a particular resource.
    REG_CMD(doRAF, "^RAF");     // Add new resources
    REG_CMD(doRFR, "^RFR");     // Force a refresh for a given resource.
    REG_CMD(doRMF, "^RMF");     // Modify an existing resource.
    REG_CMD(doRSR, "^RSR");     // Change the refresh rate for a given resource.

    REG_CMD(doABEEP, "ABEEP");  // Output a single beep even if beep is Off.
    REG_CMD(doADBEEP, "ADBEEP");// Output a double beep even if beep is Off.
    REG_CMD(doAKB, "@AKB");     // Pop up the keyboard icon and initialize the text string to that specified.
    REG_CMD(doAKEYB, "AKEYB");  // Pop up the keyboard icon and initialize the text string to that specified.
    REG_CMD(doAKP, "@AKP");     // Pop up the keypad icon and initialize the text string to that specified.
    REG_CMD(doAKEYP, "AKEYP");  // Pop up the keypad icon and initialize the text string to that specified.
    REG_CMD(doAKEYR, "AKEYR");  // Remove the Keyboard/Keypad.
    REG_CMD(doAKR, "@AKR");     // Remove the Keyboard/Keypad.
    REG_CMD(doBEEP, "BEEP");    // Play a single beep.
    REG_CMD(doBEEP, "^ABP");    // G5: Play a single beep.
    REG_CMD(doDBEEP, "DBEEP");  // Play a double beep.
    REG_CMD(doDBEEP, "^ADB");   // G5: Play a double beep.
    REG_CMD(doEKP, "@EKP");     // Pop up the keypad icon and initialize the text string to that specified.
    REG_CMD(doPKP, "@PKB");     // Present a private keyboard.
    REG_CMD(doPKP, "PKEYP");    // Present a private keypad.
    REG_CMD(doPKP, "@PKP");     // Present a private keypad.
    REG_CMD(doSetup, "SETUP");  // Send panel to SETUP page.
    REG_CMD(doSetup, "^STP");   // G5: Open setup page.
    REG_CMD(doShutdown, "SHUTDOWN");// Shut down the App
    REG_CMD(doSOU, "@SOU");     // Play a sound file.
    REG_CMD(doSOU, "^SOU");     // G5: Play a sound file.
    REG_CMD(doTKP, "@TKP");     // Present a telephone keypad.
    REG_CMD(doTKP, "^TKP");     // G5: Bring up a telephone keypad.
    REG_CMD(doTKP, "@VKB");     // Present a virtual keyboard
    REG_CMD(doTKP, "^VKB");     // G5: Bring up a virtual keyboard.
#ifndef _NOSIP_
    // Here the SIP commands will take place
    REG_CMD(doPHN, "^PHN");     // SIP commands
    REG_CMD(getPHN, "?PHN");    // SIP state commands
#endif
    // State commands
    REG_CMD(doON, "ON");
    REG_CMD(doOFF, "OFF");
    REG_CMD(doLEVEL, "LEVEL");
    REG_CMD(doBLINK, "BLINK");
    REG_CMD(doVER, "^VER?");    // Return version string to master
    REG_CMD(doWCN, "^WCN?");    // Return SIP phone number

    REG_CMD(doFTR, "#FTR");     // File transfer (virtual internal command)

    // At least we must add the SIP client
#ifndef _NOSIP_
    mSIPClient = new TSIPClient;

    if (TError::isError())
    {
        MSG_ERROR("Error initializing the SIP client!");
        TConfig::setSIPstatus(false);
    }
#endif
    TError::clear();
    surface_mutex.unlock();
}

TPageManager::~TPageManager()
{
    DECL_TRACER("TPageManager::~TPageManager()");
#ifndef _NOSIP_
    if (mSIPClient)
    {
        delete mSIPClient;
        mSIPClient = nullptr;
    }
#endif
    PCHAIN_T *p = mPchain;
    PCHAIN_T *next = nullptr;
#ifdef __ANDROID__
    stopNetworkState();
#endif
    try
    {
        while (p)
        {
            next = p->next;

            if (p->page)
                delete p->page;

            delete p;
            p = next;
        }

        SPCHAIN_T *sp = mSPchain;
        SPCHAIN_T *snext = nullptr;

        while (sp)
        {
            snext = sp->next;

            if (sp->page)
                delete sp->page;

            delete sp;
            sp = snext;
        }

        mPchain = nullptr;
        mSPchain = nullptr;
        setPChain(mPchain);
        setSPChain(mSPchain);

        if (mAmxNet)
        {
            delete mAmxNet;
            mAmxNet = nullptr;
        }

        if (mTSettings)
        {
            delete mTSettings;
            mTSettings = nullptr;
        }

        if (mPageList)
        {
            delete mPageList;
            mPageList = nullptr;
        }

        if (mPalette)
        {
            delete mPalette;
            mPalette = nullptr;
        }

        if (mFonts)
        {
            delete mFonts;
            mFonts = nullptr;
        }

        if (gIcons)
        {
            delete gIcons;
            gIcons = nullptr;
        }

        if (gPrjResources)
        {
            delete gPrjResources;
            gPrjResources = nullptr;
        }

        if (mExternal)
        {
            delete mExternal;
            mExternal = nullptr;
        }
    }
    catch (std::exception& e)
    {
        MSG_ERROR("Memory error: " << e.what());
    }

    gPageManager = nullptr;
}

void TPageManager::initialize()
{
    DECL_TRACER("TPageManager::initialize()");

    surface_mutex.lock();
    dropAllSubPages();
    dropAllPages();

    if (mAmxNet && mAmxNet->isConnected())
        mAmxNet->close();

    if (mTSettings)
        mTSettings->loadSettings();
    else
        mTSettings = new TSettings(TConfig::getProjectPath());

    if (TError::isError())
    {
        surface_mutex.unlock();
        return;
    }

    if (gPrjResources)
        delete gPrjResources;

    gPrjResources = new TPrjResources(mTSettings->getResourcesList());

    if (mPalette)
        delete mPalette;

    mPalette = new TPalette();

    vector<PALETTE_SETUP> pal = mTSettings->getSettings().palettes;

    if (pal.size() > 0)
    {
        vector<PALETTE_SETUP>::iterator iterPal;

        for (iterPal = pal.begin(); iterPal != pal.end(); ++iterPal)
            mPalette->initialize(iterPal->file);
    }

    if (!TError::isError())
        TColor::setPalette(mPalette);

    if (mFonts)
        delete mFonts;

    mFonts = new TFont();

    if (TError::isError())
    {
        MSG_ERROR("Initializing fonts was not successfull!");
        surface_mutex.unlock();
        return;
    }

    if (gIcons)
        delete gIcons;

    gIcons = new TIcons();

    if (TError::isError())
    {
        MSG_ERROR("Initializing icons was not successfull!");
        surface_mutex.unlock();
        return;
    }

    if (mPageList)
        delete mPageList;

    mPageList = new TPageList();

    if (mExternal)
        delete mExternal;

    mExternal = new TExternal();

    PAGELIST_T page;

    if (!mTSettings->getSettings().powerUpPage.empty())
    {
        if (readPage(mTSettings->getSettings().powerUpPage))
        {
            MSG_TRACE("Found power up page " << mTSettings->getSettings().powerUpPage);
            page = findPage(mTSettings->getSettings().powerUpPage);
            mActualPage = page.pageID;
        }
    }

    TPage *pg = getPage(mActualPage);

    vector<string> popups = mTSettings->getSettings().powerUpPopup;

    if (popups.size() > 0)
    {
        vector<string>::iterator iter;

        for (iter = popups.begin(); iter != popups.end(); ++iter)
        {
            if (readSubPage(*iter))
            {
                MSG_TRACE("Found power up popup " << *iter);

                if (pg)
                {
                    TSubPage *spage = getSubPage(*iter);
                    pg->addSubPage(spage);
                }
            }
        }
    }

    // Here we initialize the system resources like borders, cursors, sliders, ...
    if (mSystemDraw)
        delete mSystemDraw;

    mSystemDraw = new TSystemDraw(TConfig::getSystemPath(TConfig::BASE));

    TError::clear();        // Clear all errors who may be occured until here

    // Start the thread
    startComm();

    surface_mutex.unlock();
}

bool TPageManager::startComm()
{
    DECL_TRACER("TPageManager::startComm()");

    if (mAmxNet && mAmxNet->isNetRun())
        return true;

    try
    {
        if (!mAmxNet)
        {
            if (_netRunning)
            {
                // Wait until previous connection thread ended
                while (_netRunning)
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }

            mAmxNet = new amx::TAmxNet();
            mAmxNet->setCallback(bind(&TPageManager::doCommand, this, std::placeholders::_1));
            mAmxNet->setPanelID(TConfig::getChannel());
        }

        if (!mAmxNet->isNetRun())
            mAmxNet->Run();
    }
    catch (std::exception& e)
    {
        MSG_ERROR("Error starting the AmxNet thread: " << e.what());
        return false;
    }

    return true;
}

void TPageManager::startUp()
{
    DECL_TRACER("TPageManager::startUp()");

    if (mAmxNet)
    {
        MSG_WARNING("Communication with controller already initialized!");
        return;
    }

    if (!startComm())
        return;

#ifdef __ANDROID__
    initOrientation();
    initNetworkState();
#endif
}

void TPageManager::reset()
{
    DECL_TRACER("TPageManager::reset()");

    // Freshly initialize everything.
    initialize();
}

/*
 * The following method is called by the class TAmxNet whenever an event from
 * the controller occured.
 */
void TPageManager::doCommand(const amx::ANET_COMMAND& cmd)
{
    DECL_TRACER("TPageManager::doCommand(const amx::ANET_COMMAND& cmd)");

    mCommands.push_back(cmd);

    if (mBusy)
        return;

    mBusy = true;
    string com;

    while (mCommands.size() > 0)
    {
        amx::ANET_COMMAND& bef = mCommands.at(0);
        mCommands.erase(mCommands.begin());

        switch (bef.MC)
        {
            case 0x0006:
            case 0x0018:	// feedback channel on
                com.assign("ON-");
                com.append(to_string(bef.data.chan_state.channel));
                parseCommand(bef.device1, bef.data.chan_state.port, com);
            break;

            case 0x0007:
            case 0x0019:	// feedback channel off
                com.assign("OFF-");
                com.append(to_string(bef.data.chan_state.channel));
                parseCommand(bef.device1, bef.data.chan_state.port, com);
            break;

            case 0x000a:	// level value change
                com = "LEVEL-";
                com += to_string(bef.data.message_value.value);
                com += ",";

                switch (bef.data.message_value.type)
                {
                    case 0x10: com += to_string(bef.data.message_value.content.byte); break;
                    case 0x11: com += to_string(bef.data.message_value.content.ch); break;
                    case 0x20: com += to_string(bef.data.message_value.content.integer); break;
                    case 0x21: com += to_string(bef.data.message_value.content.sinteger); break;
                    case 0x40: com += to_string(bef.data.message_value.content.dword); break;
                    case 0x41: com += to_string(bef.data.message_value.content.sdword); break;
                    case 0x4f: com += to_string(bef.data.message_value.content.fvalue); break;
                    case 0x8f: com += to_string(bef.data.message_value.content.dvalue); break;
                }

                parseCommand(bef.device1, bef.data.chan_state.port, com);
            break;

            case 0x000c:	// Command string
            {
                amx::ANET_MSG_STRING msg = bef.data.message_string;

                if (msg.length < strlen((char *)&msg.content))
                {
                    mCmdBuffer.append((char *)&msg.content);
                    break;
                }
                else if (mCmdBuffer.length() > 0)
                {
                    mCmdBuffer.append((char *)&msg.content);
                    size_t len = (mCmdBuffer.length() >= sizeof(msg.content)) ? (sizeof(msg.content)-1) : mCmdBuffer.length();
                    strncpy((char *)&msg.content, mCmdBuffer.c_str(), len);
                    msg.content[len] = 0;
                }

                if (getCommand((char *)msg.content) == "^UTF")  // This is already UTF8!
                    com.assign((char *)msg.content);
                else
                    com.assign(cp1250ToUTF8((char *)&msg.content));

                parseCommand(bef.device1, bef.data.chan_state.port, com);
                mCmdBuffer.clear();
            }
            break;

            case 0x0502:    // Blink message (contains date and time)
                com = "BLINK-" + to_string(bef.data.blinkMessage.hour) + ":";
                com += to_string(bef.data.blinkMessage.minute) + ":";
                com += to_string(bef.data.blinkMessage.second) + ",";
                com += to_string(bef.data.blinkMessage.year) + "-";
                com += to_string(bef.data.blinkMessage.month) + "-";
                com += to_string(bef.data.blinkMessage.day) + ",";
                com += to_string(bef.data.blinkMessage.weekday) + ",";
                com += ((bef.data.blinkMessage.LED & 0x0001) ? "ON" : "OFF");
                parseCommand(0, 0, com);
            break;

            case 0x1000:	// Filetransfer
            {
                amx::ANET_FILETRANSFER ftr = bef.data.filetransfer;

                if (ftr.ftype == 0)
                {
                    switch(ftr.function)
                    {
                        case 0x0100:	// Syncing directory
                            com = "#FTR-SYNC:0:";
                            com.append((char*)&ftr.data[0]);
                            parseCommand(bef.device1, bef.port1, com);
                        break;

                        case 0x0104:	// Delete file
                            com = "#FTR-SYNC:"+to_string(bef.count)+":Deleting files ... ("+to_string(bef.count)+"%)";
                            parseCommand(bef.device1, bef.port1, com);
                        break;

                        case 0x0105:	// start filetransfer
                            com = "#FTR-START";
                            parseCommand(bef.device1, bef.port1, com);
                        break;
                    }
                }
                else
                {
                    switch(ftr.function)
                    {
                        case 0x0003:	// Received part of file
                        case 0x0004:	// End of file
                            com = "#FTR-FTRPART:"+to_string(bef.count)+":"+to_string(ftr.info1);
                            parseCommand(bef.device1, bef.port1, com);
                        break;

                        case 0x0007:	// End of file transfer
                        {
//                            initialize();
                            com = "#FTR-END";
                            parseCommand(bef.device1, bef.port1, com);
                        }
                        break;

                        case 0x0102:	// Receiving file
                            com = "#FTR-FTRSTART:"+to_string(bef.count)+":"+to_string(ftr.info1)+":";
                            com.append((char*)&ftr.data[0]);
                            parseCommand(bef.device1, bef.port1, com);
                        break;
                    }
                }
            }
            break;
        }
    }

    mBusy = false;
}

void TPageManager::deployCallbacks()
{
    DECL_TRACER("TPageManager::deployCallbacks()");

    PCHAIN_T *p = mPchain;

    while (p)
    {
        if (p->page)
        {
            if (_setBackground)
                p->page->registerCallback(_setBackground);

            if (_callPlayVideo)
                p->page->regCallPlayVideo(_callPlayVideo);
        }

        p = p->next;
    }

    SPCHAIN_T *sp = mSPchain;

    while (sp)
    {
        if (sp->page)
        {
            if (_setBackground)
                sp->page->registerCallback(_setBackground);

            if (_callPlayVideo)
                sp->page->regCallPlayVideo(_callPlayVideo);
        }

        sp = sp->next;
    }
}

void TPageManager::regCallbackNetState(std::function<void (int)> callNetState, ulong handle)
{
    DECL_TRACER("TPageManager::regCallbackNetState(std::function<void (int)> callNetState, ulong handle)");

    if (handle == 0)
        return;

    mNetCalls.insert(std::pair<int, std::function<void (int)> >(handle, callNetState));
}

void TPageManager::unregCallbackNetState(ulong handle)
{
    DECL_TRACER("TPageManager::unregCallbackNetState(ulong handle)");

    if (mNetCalls.size() == 0)
        return;

    std::map<int, std::function<void (int)> >::iterator iter = mNetCalls.find(handle);

    if (iter != mNetCalls.end())
        mNetCalls.erase(iter);
}

void TPageManager::regCallbackBatteryState(std::function<void (int, bool, int)> callBatteryState, ulong handle)
{
    DECL_TRACER("TPageManager::regCallbackBatteryState(std::function<void (int, bool, int)> callBatteryState, ulong handle)");

    if (handle == 0)
        return;

    mBatteryCalls.insert(std::pair<int, std::function<void (int, bool, int)> >(handle, callBatteryState));
}

void TPageManager::unregCallbackBatteryState(ulong handle)
{
    DECL_TRACER("TPageManager::unregCallbackBatteryState(ulong handle)");

    if (mBatteryCalls.size() == 0)
        return;

    std::map<int, std::function<void (int, bool, int)> >::iterator iter = mBatteryCalls.find(handle);

    if (iter != mBatteryCalls.end())
        mBatteryCalls.erase(iter);
}

/*
 * The following function must be called to start the "panel".
 */
bool TPageManager::run()
{
    DECL_TRACER("TPageManager::run()");

    if (mActualPage <= 0)
        return false;

    surface_mutex.lock();
    TPage *pg = getPage(mActualPage);
    pg->setFonts(mFonts);
    pg->registerCallback(_setBackground);
    pg->regCallPlayVideo(_callPlayVideo);

    if (!pg || !_setPage || !mTSettings)
    {
        surface_mutex.unlock();
        return false;
    }

    int width, height;
    width = mTSettings->getWith();
    height = mTSettings->getHeight();
#ifdef _SCALE_SKIA_
    if (mScaleFactor != 1.0)
    {
        width = (int)((double)width * mScaleFactor);
        height = (int)((double)height * mScaleFactor);
    }
#endif
    _setPage((pg->getNumber() << 16) & 0xffff0000, width, height);
    pg->show();

    TSubPage *subPg = pg->getFirstSubPage();

    while (subPg)
    {
        subPg->setFonts(mFonts);
        subPg->registerCallback(_setBackground);
        subPg->registerCallbackDB(_displayButton);
        subPg->regCallDropSubPage(_callDropSubPage);
        subPg->regCallPlayVideo(_callPlayVideo);

        if (_setSubPage)
        {
            MSG_DEBUG("Drawing page " << subPg->getNumber() << ": " << subPg->getName() << "...");
            width = subPg->getWidth();
            height = subPg->getHeight();
            int left = subPg->getLeft();
            int top = subPg->getTop();
#ifdef _SCALE_SKIA_
            if (mScaleFactor != 1.0)
            {
                width = (int)((double)width * mScaleFactor);
                height = (int)((double)height * mScaleFactor);
                left = (int)((double)left * mScaleFactor);
                top = (int)((double)top * mScaleFactor);
            }
#endif
            ANIMATION_t ani;
            ani.showEffect = subPg->getShowEffect();
            ani.showTime = subPg->getShowTime();
            ani.hideEffect = subPg->getHideEffect();
            ani.hideTime = subPg->getHideTime();
            _setSubPage(((subPg->getNumber() << 16) & 0xffff0000), left, top, width, height, ani);
            subPg->show();
        }

        subPg = pg->getNextSubPage();
    }

    surface_mutex.unlock();
    return true;
}

TPage *TPageManager::getPage(int pageID)
{
    DECL_TRACER("TPageManager::getPage(int pageID)");

    PCHAIN_T *p = mPchain;

    while (p)
    {
        if (p->page->getNumber() == pageID)
            return p->page;

        p = p->next;
    }

    MSG_DEBUG("Page " << pageID << " not found!");
    return nullptr;
}

TPage *TPageManager::getPage(const string& name)
{
    DECL_TRACER("TPageManager::getPage(const string& name)");

    PCHAIN_T *p = mPchain;

    while (p)
    {
        if (p->page->getName().compare(name) == 0)
            return p->page;

        p = p->next;
    }

    MSG_DEBUG("Page " << name << " not found!");
    return nullptr;
}

TPage *TPageManager::loadPage(PAGELIST_T& pl)
{
    DECL_TRACER("TPageManager::loadPage(PAGELIST_T& pl)");

    if (!pl.isValid)
        return nullptr;

    TPage *pg = getPage(pl.pageID);

    if (!pg)
    {
        if (!readPage(pl.pageID))
            return nullptr;

        pg = getPage(pl.pageID);

        if (!pg)
        {
            MSG_ERROR("Error loading page " << pl.pageID << ", " << pl.name << " from file " << pl.file << "!");
            return nullptr;
        }
    }

    return pg;
}

bool TPageManager::setPage(int PageID)
{
    DECL_TRACER("TPageManager::setPage(int PageID)");

    if (mActualPage == PageID)
        return true;

    TPage *pg = getPage(mActualPage);
    // FIXME: Make this a vector array to hold a larger history!
    mPreviousPage = mActualPage;

    if (pg)
        pg->drop();

    mActualPage = 0;
    PAGELIST_T listPg = findPage(PageID);

    if ((pg = loadPage(listPg)) == nullptr)
        return false;

    mActualPage = PageID;
    pg->show();
    return true;
}

bool TPageManager::setPage(const string& name)
{
    DECL_TRACER("TPageManager::setPage(const string& name)");

    TPage *pg = getPage(mActualPage);

    if (pg && pg->getName().compare(name) == 0)
        return true;

    // FIXME: Make this a vector array to hold a larger history!
    mPreviousPage = mActualPage;    // Necessary to be able to jump back to at least the last previous page

    if (pg)
        pg->drop();

    mActualPage = 0;
    PAGELIST_T listPg = findPage(name);

    if ((pg = loadPage(listPg)) == nullptr)
        return false;

    mActualPage = pg->getNumber();
    pg->show();
    return true;
}

TSubPage *TPageManager::getSubPage(int pageID)
{
    DECL_TRACER("TPageManager::getSubPage(int pageID)");

    SPCHAIN_T *p = mSPchain;

    while(p)
    {
        if (p->page->getNumber() == pageID)
            return p->page;

        p = p->next;
    }

    return nullptr;
}

TSubPage *TPageManager::getSubPage(const std::string& name)
{
    DECL_TRACER("TPageManager::getSubPage(const std::string& name)");

    SPCHAIN_T *p = mSPchain;

    while (p)
    {
        if (p->page->getName().compare(name) == 0)
            return p->page;

        p = p->next;
    }

    return nullptr;
}

TSubPage *TPageManager::deliverSubPage(const string& name, TPage **pg)
{
    DECL_TRACER("TPageManager::deliverSubPage(const string& name)");

    TPage *page = getActualPage();

    if (!page)
    {
        MSG_ERROR("No actual page loaded!");
        return nullptr;
    }

    if (pg)
        *pg = page;

    TSubPage *subPage = getSubPage(name);

    if (!subPage)
    {
        if (!readSubPage(name))
        {
            MSG_ERROR("Error reading subpage " << name);
            return nullptr;
        }

        subPage = getSubPage(name);

        if (!subPage)
        {
            MSG_ERROR("Fatal: A page with name " << name << " does not exist!");
            return nullptr;
        }

        page->addSubPage(subPage);
    }

    return subPage;
}

bool TPageManager::readPages()
{
    DECL_TRACER("TPageManager::readPages()");

    if (!mPageList)
    {
        MSG_ERROR("Page list is not initialized!");
        TError::setError();
        return false;
    }

    // Read all pages
    vector<PAGELIST_T> pageList = mPageList->getPagelist();

    if (pageList.size() > 0)
    {
        vector<PAGELIST_T>::iterator pgIter;

        for (pgIter = pageList.begin(); pgIter != pageList.end(); ++pgIter)
        {
            TPage *page = new TPage(pgIter->name+".xml");

            if (TError::isError())
            {
                delete page;
                return false;
            }

            page->setPalette(mPalette);
            page->setFonts(mFonts);
            page->registerCallback(_setBackground);
            page->registerCallbackDB(_displayButton);
            page->regCallPlayVideo(_callPlayVideo);

            if (!addPage(page))
                return false;
        }
    }

    vector<SUBPAGELIST_T> subPageList = mPageList->getSupPageList();

    if (subPageList.size() > 0)
    {
        vector<SUBPAGELIST_T>::iterator spgIter;

        for (spgIter = subPageList.begin(); spgIter != subPageList.end(); ++spgIter)
        {
            TSubPage *page = new TSubPage(spgIter->name+".xml");

            if (TError::isError())
            {
                delete page;
                return false;
            }

            page->setPalette(mPalette);
            page->setFonts(mFonts);
            page->registerCallback(_setBackground);
            page->registerCallbackDB(_displayButton);
            page->regCallDropSubPage(_callDropSubPage);
            page->regCallPlayVideo(_callPlayVideo);
            page->setGroup(spgIter->group);

            if (!addSubPage(page))
                return false;
        }
    }

    return true;
}

bool TPageManager::readPage(const std::string& name)
{
    DECL_TRACER("TPageManager::readPage(const std::string& name)");

    PAGELIST_T page = findPage(name);

    if (page.pageID <= 0)
    {
        MSG_ERROR("Page " << name << " not found!");
        return false;
    }

    TPage *pg;

    if (name.compare("_progress") == 0)
        pg = new TPage(name);
    else
        pg = new TPage(page.name+".xml");

    if (TError::isError())
    {
        delete pg;
        return false;
    }

    pg->setPalette(mPalette);
    pg->setFonts(mFonts);
    pg->registerCallback(_setBackground);
    pg->registerCallbackDB(_displayButton);
    pg->regCallPlayVideo(_callPlayVideo);

    if (!addPage(pg))
        return false;

    return true;
}

bool TPageManager::readPage(int ID)
{
    DECL_TRACER("TPageManager::readPage(int ID)");

    TError::clear();
    PAGELIST_T page = findPage(ID);

    if (page.pageID <= 0)
    {
        MSG_ERROR("Page with ID " << ID << " not found!");
        return false;
    }

    TPage *pg;

    if (ID == 300)      // Progress page of system?
        pg = new TPage("_progress");
    else
        pg = new TPage(page.name+".xml");

    if (TError::isError())
    {
        delete pg;
        return false;
    }

    pg->setPalette(mPalette);
    pg->setFonts(mFonts);
    pg->registerCallback(_setBackground);
    pg->registerCallbackDB(_displayButton);
    pg->regCallPlayVideo(_callPlayVideo);

    if (!addPage(pg))
        return false;

    return true;
}

bool TPageManager::readSubPage(const std::string& name)
{
    DECL_TRACER("TPageManager::readSubPage(const std::string& name)");

    TError::clear();
    SUBPAGELIST_T page = findSubPage(name);

    if (page.pageID <= 0)
    {
        MSG_ERROR("Subpage " << name << " not found!");
        return false;
    }

    if (haveSubPage(name))
        return true;

    TSubPage *pg = new TSubPage(page.name+".xml");

    if (TError::isError())
    {
        delete pg;
        return false;
    }

    pg->setPalette(mPalette);
    pg->setFonts(mFonts);
    pg->registerCallback(_setBackground);
    pg->registerCallbackDB(_displayButton);
    pg->regCallDropSubPage(_callDropSubPage);
    pg->regCallPlayVideo(_callPlayVideo);
    pg->setGroup(page.group);

    if (!addSubPage(pg))
    {
        delete pg;
        return false;
    }

    return true;
}

bool TPageManager::readSubPage(int ID)
{
    DECL_TRACER("TPageManager::readSubPage(int ID)");

    TError::clear();
    SUBPAGELIST_T page = findSubPage(ID);

    if (page.pageID <= 0)
    {
        MSG_ERROR("Subpage with ID " << ID << " not found!");
        return false;
    }

    TSubPage *pg = new TSubPage(page.name+".xml");

    if (TError::isError())
    {
        delete pg;
        return false;
    }

    pg->setPalette(mPalette);
    pg->setFonts(mFonts);
    pg->registerCallback(_setBackground);
    pg->registerCallbackDB(_displayButton);
    pg->regCallDropSubPage(_callDropSubPage);
    pg->regCallPlayVideo(_callPlayVideo);
    pg->setGroup(page.group);

    if (!addSubPage(pg))
        return false;

    return true;
}

/******************** Internal private methods *********************/

PAGELIST_T TPageManager::findPage(const std::string& name)
{
    DECL_TRACER("TPageManager::findPage(const std::string& name)");

    vector<PAGELIST_T> pageList = mPageList->getPagelist();

    if (pageList.size() > 0)
    {
        vector<PAGELIST_T>::iterator pgIter;

        for (pgIter = pageList.begin(); pgIter != pageList.end(); ++pgIter)
        {
            if (pgIter->name.compare(name) == 0)
                return *pgIter;
        }
    }

    return PAGELIST_T();
}

PAGELIST_T TPageManager::findPage(int ID)
{
    DECL_TRACER("TPageManager::findPage(int ID)");

    vector<PAGELIST_T> pageList = mPageList->getPagelist();

    if (pageList.size() > 0)
    {
        vector<PAGELIST_T>::iterator pgIter;

        for (pgIter = pageList.begin(); pgIter != pageList.end(); ++pgIter)
        {
            if (pgIter->pageID == ID)
                return *pgIter;
        }
    }

    return PAGELIST_T();
}

SUBPAGELIST_T TPageManager::findSubPage(const std::string& name)
{
    DECL_TRACER("TPageManager::findSubPage(const std::string& name)");

    vector<SUBPAGELIST_T> pageList = mPageList->getSupPageList();

    if (pageList.size() > 0)
    {
        vector<SUBPAGELIST_T>::iterator pgIter;

        for (pgIter = pageList.begin(); pgIter != pageList.end(); ++pgIter)
        {
            if (pgIter->name.compare(name) == 0)
                return *pgIter;
        }
    }

    return SUBPAGELIST_T();
}

SUBPAGELIST_T TPageManager::findSubPage(int ID)
{
    DECL_TRACER("TPageManager::findSubPage(int ID)");

    vector<SUBPAGELIST_T> pageList = mPageList->getSupPageList();

    if (pageList.size() > 0)
    {
        vector<SUBPAGELIST_T>::iterator pgIter;

        for (pgIter = pageList.begin(); pgIter != pageList.end(); ++pgIter)
        {
            if (pgIter->pageID == ID)
                return *pgIter;
        }
    }

    return SUBPAGELIST_T();
}

bool TPageManager::addPage(TPage* pg)
{
    DECL_TRACER("TPageManager::addPage(TPage* pg)");

    if (!pg)
    {
        MSG_ERROR("Parameter is NULL!");
        TError::setError();
        return false;
    }

    PCHAIN_T *chain = new PCHAIN_T;
    chain->page = pg;
    chain->next = nullptr;

    if (mPchain)
    {
        PCHAIN_T *p = mPchain;

        while (p->next)
            p = p->next;

        p->next = chain;
    }
    else
    {
        mPchain = chain;
        setPChain(mPchain);
    }

    MSG_DEBUG("Added page " << chain->page->getName());
    return true;
}

bool TPageManager::addSubPage(TSubPage* pg)
{
    DECL_TRACER("TPageManager::addSubPage(TSubPage* pg)");

    if (!pg)
    {
        MSG_ERROR("Parameter is NULL!");
        TError::setError();
        return false;
    }

    if (haveSubPage(pg->getNumber()))
    {
        MSG_ERROR("Subpage " << pg->getNumber() << ", " << pg->getName() << " is already in chain!");
        return false;
    }

    SPCHAIN_T *chain = new SPCHAIN_T;
    chain->page = pg;
    chain->next = nullptr;

    if (mSPchain)
    {
        SPCHAIN_T *p = mSPchain;

        while (p->next)
            p = p->next;

        p->next = chain;
    }
    else
    {
        mSPchain = chain;
        setSPChain(mSPchain);
    }

    MSG_DEBUG("Added subpage " << chain->page->getName());
    return true;
}

void TPageManager::dropAllPages()
{
    DECL_TRACER("TPageManager::dropAllPages()");

    PCHAIN_T *pg = mPchain;
    PCHAIN_T *next = nullptr;

    while (pg)
    {
        next = pg->next;

        if (pg->page)
        {
            if (_callDropPage)
                _callDropPage((pg->page->getNumber() << 16) & 0xffff0000);

            delete pg->page;
        }

        delete pg;
        pg = next;
    }

    mPchain = nullptr;
    setPChain(mPchain);
}

void TPageManager::dropAllSubPages()
{
    DECL_TRACER("TPageManager::dropAllSubPages()");

    SPCHAIN_T *spg = mSPchain;
    SPCHAIN_T *next;

    while (spg)
    {
        next = spg->next;

        if (spg->page)
        {
            if (_callDropSubPage)
                _callDropSubPage((spg->page->getNumber() << 16) & 0xffff0000);

            delete spg->page;
        }

        delete spg;
        spg = next;
    }

    mSPchain = nullptr;
    setSPChain(mSPchain);
}

bool TPageManager::destroyAll()
{
    DECL_TRACER("TPageManager::destroyAll()");

    dropAllSubPages();
    dropAllPages();
    mActualPage = 0;
    mPreviousPage = 0;
    mActualGroupName.clear();

    if (mPageList)
    {
        delete mPageList;
        mPageList = nullptr;
    }

    if (mTSettings)
    {
        delete mTSettings;
        mTSettings = nullptr;
    }

    if (mPalette)
    {
        delete mPalette;
        mPalette = nullptr;
    }

    if (mFonts)
    {
        delete mFonts;
        mFonts = nullptr;
    }

    if (mExternal)
    {
        delete mExternal;
        mExternal = nullptr;
    }

    if (gPrjResources)
    {
        delete gPrjResources;
        gPrjResources = nullptr;
    }

    if (gIcons)
    {
        delete gIcons;
        gIcons = nullptr;
    }

    if (TError::isError())
        return false;

    return true;
}

Button::TButton *TPageManager::findButton(ulong handle)
{
    DECL_TRACER("TPageManager::findButton(ulong handle)");

    TPage *pg = getPage(mActualPage);

    if (!pg)
        return nullptr;

    vector<Button::TButton *> pgBtList = pg->getAllButtons();
    vector<Button::TButton *>::iterator iter;

    if (pgBtList.size() > 0)
    {
        // First we look into the elements of the page
        for (iter = pgBtList.begin(); iter != pgBtList.end(); ++iter)
        {
            Button::TButton *bt = *iter;

            if (bt->getHandle() == handle)
                return bt;
        }
    }

    // We've not found the wanted element in the elements of the page. So
    // we're looking at the elements of the subpages.
    TSubPage *sp = pg->getFirstSubPage();

    if (!sp)
        return nullptr;

    while (sp)
    {
        vector<Button::TButton *> spBtList = sp->getAllButtons();

        if (spBtList.size() > 0)
        {
            for (iter = spBtList.begin(); iter != spBtList.end(); ++iter)
            {
                Button::TButton *bt = *iter;

                if (bt->getHandle() == handle)
                    return bt;
            }
        }

        sp = pg->getNextSubPage();
    }

    return nullptr;
}

TPage *TPageManager::getActualPage()
{
    return getPage(mActualPage);
}

TSubPage *TPageManager::getFirstSubPage()
{
    DECL_TRACER("TPageManager::getFirstSubPage()");
    TPage *pg = getPage(mActualPage);

    if (!pg)
        return nullptr;

    return pg->getFirstSubPage();
}

TSubPage *TPageManager::getNextSubPage()
{
    DECL_TRACER("TPageManager::getNextSubPage()");

    TPage *pg = getPage(mActualPage);

    if (pg)
        return pg->getNextSubPage();

    return nullptr;
}

TSubPage *TPageManager::getFirstSubPageGroup(const string& group)
{
    DECL_TRACER("TPageManager::getFirstSubPageGroup(const string& group)");

    if (group.empty())
    {
        MSG_WARNING("Empty group name is invalid. Ignoring it!");
        mActualGroupName.clear();
        mActualGroupPage = nullptr;
        return nullptr;
    }

    mActualGroupName = group;
    TSubPage *pg = getFirstSubPage();

    while (pg)
    {
        MSG_DEBUG("Evaluating group " << pg->getGroupName() << " with " << group);

        if (pg->getGroupName().compare(group) == 0)
        {
            mActualGroupPage = pg;
            return pg;
        }

        pg = getNextSubPage();
    }

    mActualGroupName.clear();
    mActualGroupPage = nullptr;
    return nullptr;
}

TSubPage *TPageManager::getNextSubPageGroup()
{
    DECL_TRACER("TPageManager::getNextSubPageGroup()");

    if (mActualGroupName.empty())
        return nullptr;

    TSubPage *pg = getFirstSubPage();
    bool found = false;

    while (pg)
    {
        MSG_DEBUG("Evaluating group " << pg->getGroupName() << " with " << mActualGroupName);

        if (!found && pg == mActualGroupPage)
        {
            pg = getNextSubPage();
            found = true;
            continue;
        }

        if (found && pg->getGroupName().compare(mActualGroupName) == 0)
        {
            mActualGroupPage = pg;
            return pg;
        }

        pg = getNextSubPage();
    }

    mActualGroupName.clear();
    mActualGroupPage = nullptr;
    return nullptr;
}

TSubPage *TPageManager::getNextSubPageGroup(const string& group, TSubPage* pg)
{
    DECL_TRACER("TPageManager::getNextSubPageGroup(const string& group, TSubPage* pg)");

    if (group.empty() || !pg)
        return nullptr;

    TSubPage *page = getFirstSubPage();
    bool found = false;

    while (page)
    {
        MSG_DEBUG("Evaluating group " << pg->getGroupName() << " with " << group);

        if (!found && pg == page)
        {
            page = getNextSubPage();
            found = true;
            continue;
        }

        if (found && page->getGroupName().compare(group) == 0)
            return page;

        page = getNextSubPage();
    }

    return nullptr;
}

TSubPage *TPageManager::getTopPage()
{
    DECL_TRACER("TPageManager::getTopPage()");

    // Scan for all occupied regions
    vector<RECT_T> regions;

    TSubPage *pg = getFirstSubPage();

    while (pg)
    {
        RECT_T r = pg->getRegion();
        regions.push_back(r);
        pg = getNextSubPage();
    }

    // Now scan all pages against all regions to find the top most
    pg = getFirstSubPage();
    TSubPage *top = nullptr;
    int zPos = 0;

    while (pg)
    {
        RECT_T r = pg->getRegion();

        if (regions.size() > 0)
        {
            vector<RECT_T>::iterator iter;
            int zo = 0;

            for (iter = regions.begin(); iter != regions.end(); ++iter)
            {
                if (doOverlap(*iter, r) && zPos > zo)
                    top = pg;

                zo++;
            }
        }

        pg = getNextSubPage();
        zPos++;
    }

    return top;
}

TSubPage *TPageManager::getCoordMatch(int x, int y)
{
    DECL_TRACER("TPageManager::getCoordMatch(int x, int y)");

    int realX = x;
    int realY = y;

    // Reverse order of pages
    map<int, TSubPage *> zOrder;
    TSubPage *pg = getFirstSubPage();

    while (pg)
    {
        if (pg->isVisible())
            zOrder.insert(pair<int, TSubPage *>(pg->getZOrder(), pg));

        pg = getNextSubPage();
    }

    // Iterate in reverse order through array
    if (zOrder.size() > 0)
    {
        map<int, TSubPage *>::reverse_iterator iter;

        for (iter = zOrder.rbegin(); iter != zOrder.rend(); ++iter)
        {
            RECT_T r = iter->second->getRegion();

            if (r.left <= realX && (r.left + r.width) >= realX &&
                r.top <= realY && (r.top + r.height) >= realY)
            {
                MSG_DEBUG("Click matches subpage " << iter->second->getNumber() << " (" << iter->second->getName() << ")");
                return iter->second;
            }
        }
    }

    return nullptr;
}

Button::TButton *TPageManager::getCoordMatchPage(int x, int y)
{
    DECL_TRACER("TPageManager::getCoordMatchPage(int x, int y)");

    TPage *page = getActualPage();

    if (page)
    {
        Button::TButton *bt = page->getFirstButton();

        while (bt)
        {
            MSG_DEBUG("Button: " << bt->getButtonIndex() << ", l: " << bt->getLeftPosition() << ", t: " << bt->getTopPosition() << ", w: " << bt->getWidth() << ", h: " << bt->getHeight() << ", x: " << x << ", y: " << y);

            if (bt->getLeftPosition() <= x && (bt->getLeftPosition() + bt->getWidth()) >= x &&
                bt->getTopPosition() <= y && (bt->getTopPosition() + bt->getHeight()) >= y)
            {
                MSG_DEBUG("Click matches button " << bt->getButtonIndex() << " (" << bt->getButtonName() << ")");
                return bt;
            }

            bt = page->getNextButton();
        }
    }

    return nullptr;
}

bool TPageManager::doOverlap(RECT_T r1, RECT_T r2)
{
    DECL_TRACER("TPageManager::doOverlap(RECT_T r1, RECT_T r2)");

    // If one rectangle is on left side of other
    if (r1.left >= r2.left || r2.left >= r1.left)
        return false;

    // If one rectangle is above other
    if (r1.top <= r2.top || r2.top <= r1.top)
        return false;

    return true;
}

bool TPageManager::havePage(const string& name)
{
    DECL_TRACER("TPageManager::havePage(const string& name)");

    if (name.empty())
        return false;

    PCHAIN_T *pg = mPchain;

    while (pg)
    {
        if (pg->page && pg->page->getName().compare(name) == 0)
            return true;

        pg = pg->next;
    }

    return false;
}

bool TPageManager::haveSubPage(const string& name)
{
    DECL_TRACER("TPageManager::haveSubPage(const string& name)");

    if (name.empty())
        return false;

    SPCHAIN_T *pg = mSPchain;

    while (pg)
    {
        if (pg->page && pg->page->getName().compare(name) == 0)
        {
            MSG_DEBUG("Subpage " << pg->page->getNumber() << ", " << name << " found.");
            return true;
        }

        pg = pg->next;
    }

    MSG_DEBUG("Subpage " << name << " not found.");
    return false;
}

bool TPageManager::haveSubPage(int id)
{
    DECL_TRACER("TPageManager::haveSubPage(int id)");

    SPCHAIN_T *pg = mSPchain;

    while (pg)
    {
        if (pg->page && pg->page->getNumber() == id)
        {
            MSG_DEBUG("Subpage " << pg->page->getNumber() << ", " << pg->page->getName() << " found.");
            return true;
        }

        pg = pg->next;
    }

    MSG_DEBUG("Subpage " << id << " not found.");
    return false;
}

bool TPageManager::haveSubPage(const string& page, const string& name)
{
    DECL_TRACER("TPageManager::haveSubPage(const string& page, const string& name)");

    TPage *pg = getPage(page);

    if (!pg)
        return false;

    TSubPage *spg = pg->getFirstSubPage();

    while (spg)
    {
        if (spg->getName().compare(name) == 0)
        {
            MSG_DEBUG("Subpage " << spg->getNumber() << ", " << name << " found.");
            return true;
        }

        spg = pg->getNextSubPage();
    }

    MSG_DEBUG("Subpage " << name << " not found on page " << page << ".");
    return false;
}

bool TPageManager::haveSubPage(const string& page, int id)
{
    DECL_TRACER("TPageManager::haveSubPage(const string& page, int id)");

    TPage *pg = getPage(page);

    if (!pg)
        return false;

    TSubPage *spg = pg->getFirstSubPage();

    while (spg)
    {
        if (spg->getNumber() == id)
        {
            MSG_DEBUG("Subpage " << spg->getNumber() << ", " << spg->getName() << " found.");
            return true;
        }

        spg = pg->getNextSubPage();
    }

    MSG_DEBUG("Subpage " << id << " on page " << page << " not found.");
    return false;
}

void TPageManager::closeGroup(const string& group)
{
    DECL_TRACER("TPageManager::closeGroup(const string& group)");

    SPCHAIN_T *pg = mSPchain;

    while (pg)
    {
        if (pg->page->getGroupName().compare(group) == 0 && pg->page->isVisible())
        {
            pg->page->regCallDropSubPage(_callDropSubPage);
            pg->page->drop();
            break;
        }

        pg = pg->next;
    }
}

void TPageManager::showSubPage(const string& name)
{
    DECL_TRACER("TPageManager::showSubPage(const string& name)");

    if (name.empty())
        return;

    TSubPage *pg = deliverSubPage(name);

    if (!pg)
        return;

    string group = pg->getGroupName();

    if (!group.empty())
    {
        TSubPage *sub = getFirstSubPageGroup(group);

        while(sub)
        {
            if (sub->isVisible() && sub->getNumber() != pg->getNumber())
                sub->drop();

            sub = getNextSubPageGroup(group, sub);
        }
    }

    if (!pg->isVisible())
    {
        TPage *page = getPage(mActualPage);

        if (!page)
        {
            MSG_ERROR("No active page found! Internal error.");
            return;
        }

        if (!haveSubPage(pg->getNumber()) && !page->addSubPage(pg))
            return;

        pg->setZOrder(page->getNextZOrder());

        if (_setSubPage)
        {
            int left = pg->getLeft();
            int top = pg->getTop();
            int width = pg->getWidth();
            int height = pg->getHeight();
#ifdef _SCALE_SKIA_
            if (mScaleFactor != 1.0)
            {
                left = (int)((double)left * mScaleFactor);
                top = (int)((double)top * mScaleFactor);
                width = (int)((double)width * mScaleFactor);
                height = (int)((double)height * mScaleFactor);
                MSG_DEBUG("Scaled subpage: left=" << left << ", top=" << top << ", width=" << width << ", height=" << height);
            }
#endif
            ANIMATION_t ani;
            ani.showEffect = pg->getShowEffect();
            ani.showTime = pg->getShowTime();
            ani.hideEffect = pg->getHideEffect();
            ani.hideTime = pg->getHideTime();
            // Test for a timer on the page
            if (pg->getTimeout() > 0)
                pg->startTimer();

            _setSubPage((pg->getNumber() << 16) & 0xffff0000, left, top, width, height, ani);
        }
    }

    pg->show();
}

void TPageManager::hideSubPage(const string& name)
{
    DECL_TRACER("TPageManager::hideSubPage(const string& name)");

    if (name.empty())
        return;

    TPage *page = getPage(mActualPage);

    if (!page)
    {
        MSG_ERROR("No active page found! Internal error.");
        return;
    }

    TSubPage *pg = getSubPage(name);

    if (pg)
    {
        if (pg->getZOrder() == page->getActZOrder())
            page->decZOrder();

        pg->drop();
    }
}

/*
 * Catch the mouse presses and scan all pages and subpages for an element to
 * receive the klick.
 */
void TPageManager::mouseEvent(int x, int y, bool pressed)
{
    DECL_TRACER("TPageManager::mouseEvent(int x, int y, bool pressed)");

    TError::clear();
    int realX = x - mFirstLeftPixel;
    int realY = y - mFirstTopPixel;
    MSG_DEBUG("Mouse at " << realX << ", " << realY << ", state " << ((pressed) ? "PRESSED" : "RELEASED") << ", [ " << x << " | " << y << " ]");
#ifdef _SCALE_SKIA_
    if (mScaleFactor != 1.0 && mScaleFactor > 0.0)
    {
        realX = (int)((double)realX / mScaleFactor);
        realY = (int)((double)realY / mScaleFactor);
        MSG_DEBUG("Scaled coordinates: x=" << realX << ", y=" << realY);
    }
#endif

    TSubPage *subPage = getCoordMatch(realX, realY);

    if (!subPage)
    {
        Button::TButton *bt = getCoordMatchPage(x, y);

        if (bt)
        {
            MSG_DEBUG("Button on page " << bt->getButtonIndex() << ": size: left=" << bt->getLeftPosition() << ", top=" << bt->getTopPosition() << ", width=" << bt->getWidth() << ", height=" << bt->getHeight());
            TSystemSound sysSound(TConfig::getSystemPath(TConfig::SOUNDS));

            if (pressed && _playSound && sysSound.getSystemSoundState())
                _playSound(sysSound.getTouchFeedbackSound());

            bt->doClick(x - bt->getLeftPosition(), y - bt->getTopPosition(), pressed);
        }

        return;
    }

    MSG_DEBUG("Subpage " << subPage->getNumber() << ": size: left=" << subPage->getLeft() << ", top=" << subPage->getTop() << ", width=" << subPage->getWidth() << ", height=" << subPage->getHeight());
    subPage->doClick(realX - subPage->getLeft(), realY - subPage->getTop(), pressed);
}

void TPageManager::setTextToButton(ulong handle, const string& txt)
{
    DECL_TRACER("TPageManager::setTextToButton(ulong handle, const string& txt)");

    // First we search for the button the handle points to
    Button::TButton *button = findButton(handle);

    if (!button)
    {
        MSG_ERROR("No button with handle " << TObject::handleToString(handle) << " found!");
        return;
    }

    // Now we search for all buttons with the same channel and port number
    vector<int> channels;
    channels.push_back(button->getAddressChannel());
    vector<MAP_T> map = findButtons(button->getAddressPort(), channels);

    if (TError::isError() || map.empty())
        return;

    // Here we load all buttons found.
    vector<Button::TButton *> buttons = collectButtons(map);

    if (buttons.size() > 0)
    {
        vector<Button::TButton *>::iterator mapIter;
        // Finaly we iterate through all found buttons and set the text
        for (mapIter = buttons.begin(); mapIter != buttons.end(); ++mapIter)
        {
            Button::TButton *bt = *mapIter;

            int bst = bt->getNumberInstances();

            for (int i = 0; i < bst; i++)
                bt->setTextOnly(txt, i);
        }
    }
}

vector<Button::TButton *> TPageManager::collectButtons(vector<MAP_T>& map)
{
    DECL_TRACER("TPageManager::collectButtons(vector<MAP_T>& map)");

    vector<Button::TButton *> buttons;

    if (map.size() == 0)
        return buttons;

    vector<MAP_T>::iterator iter;

    for (iter = map.begin(); iter != map.end(); ++iter)
    {
        if (iter->pg < 500)     // Main page?
        {
            TPage *page;

            if ((page = getPage(iter->pg)) == nullptr)
            {
                MSG_TRACE("Page " << iter->pg << ", " << iter->pn << " not found in memory. Reading it ...");

                if (!readPage(iter->pg))
                    return buttons;

                page = getPage(iter->pg);
                addPage(page);
            }

            Button::TButton *bt = page->getButton(iter->bt);

            if (bt)
                buttons.push_back(bt);
        }
        else
        {
            TSubPage *subpage;

            if ((subpage = getSubPage(iter->pg)) == nullptr)
            {
                MSG_TRACE("Subpage " << iter->pg << ", " << iter->pn << " not found in memory. Reading it ...");

                if (!readSubPage(iter->pg))
                    return buttons;

                subpage = getSubPage(iter->pg);
                TPage *page = getActualPage();

                if (!page)
                {
                    MSG_ERROR("No actual page loaded!");
                    return buttons;
                }

                page->addSubPage(subpage);
            }

            Button::TButton *bt = subpage->getButton(iter->bt);

            if (bt)
                buttons.push_back(bt);
        }
    }

    return buttons;
}

/****************************************************************************
 * Calls from a Java activity. This is only available for Android OS.
 ****************************************************************************/
#ifdef __ANDROID__
void TPageManager::initNetworkState()
{
    DECL_TRACER("TPageManager::initNetworkState()");

    QAndroidJniObject activity = QtAndroid::androidActivity();
    QAndroidJniObject::callStaticMethod<void>("org/qtproject/theosys/NetworkStatus", "Init", "(Landroid/app/Activity;)V", activity.object());
    activity.callStaticMethod<void>("org/qtproject/theosys/NetworkStatus", "InstallNetworkListener", "()V");
}

void TPageManager::stopNetworkState()
{
    DECL_TRACER("TPageManager::stopNetworkState()");

    QAndroidJniObject::callStaticMethod<void>("org/qtproject/theosys/NetworkStatus", "destroyNetworkListener", "()V");
}

void TPageManager::initBatteryState()
{
    DECL_TRACER("TPageManager::initBatteryState()");

    QAndroidJniObject activity = QtAndroid::androidActivity();
    QAndroidJniObject::callStaticMethod<void>("org/qtproject/theosys/BatteryState", "Init", "(Landroid/app/Activity;)V", activity.object());
    activity.callStaticMethod<void>("org/qtproject/theosys/BatteryState", "InstallBatteryListener", "()V");
}

void TPageManager::initPhoneState()
{
    DECL_TRACER("TPageManager::initPhoneState()");

    QAndroidJniObject activity = QtAndroid::androidActivity();
    QAndroidJniObject::callStaticMethod<void>("org/qtproject/theosys/PhoneCallState", "Init", "(Landroid/app/Activity;)V", activity.object());
    activity.callStaticMethod<void>("org/qtproject/theosys/PhoneCallState", "InstallPhoneListener", "()V");
}

void TPageManager::stopBatteryState()
{
    DECL_TRACER("TPageManager::stopBatteryState()");

    QAndroidJniObject::callStaticMethod<void>("org/qtproject/theosys/BatteryState", "destroyBatteryListener", "()V");
}

void TPageManager::informTPanelNetwork(jboolean conn, jint level, jint type)
{
    DECL_TRACER("TPageManager::informTPanelNetwork(jboolean conn)");

    int l = 0;
    string sType;

    switch (type)
    {
        case 1: sType = "Wifi"; break;
        case 2: sType = "Mobile"; break;

        default:
            sType = "Unknown"; break;
    }

    if (conn)
        l = level;

    if (mNetState && mNetState != type)     // Has the connection type changed?
    {
        if (gAmxNet)
            gAmxNet->reconnect();
    }

    mNetState = type;

    MSG_INFO("Connection status: " << (conn ? "Connected" : "Disconnected") << ", level: " << level << ", type: " << sType);

    if (mNetCalls.size() > 0)
    {
        std::map<int, std::function<void (int level)> >::iterator iter;

        for (iter = mNetCalls.begin(); iter != mNetCalls.end(); ++iter)
            iter->second(l);
    }
}

void TPageManager::informBatteryStatus(jint level, jboolean charging, jint chargeType)
{
    DECL_TRACER("TPageManager::informBatteryStatus(jint level, jboolean charging, jint chargeType)");

    MSG_INFO("Battery status: level: " << level << ", " << (charging ? "Charging" : "not charging") << ", type: " << chargeType << ", Elements: " << mBatteryCalls.size());

    if (mBatteryCalls.size() > 0)
    {
        std::map<int, std::function<void (int, bool, int)> >::iterator iter;

        for (iter = mBatteryCalls.begin(); iter != mBatteryCalls.end(); ++iter)
            iter->second(level, charging, chargeType);
    }
}

void TPageManager::informPhoneState(bool call, const string &pnumber)
{
    DECL_TRACER("TPageManager::informPhoneState(bool call, const string &pnumber)");

    MSG_INFO("Call state: " << (call ? "Call in progress" : "No call") << ", phone number: " << pnumber);

    if (!gAmxNet)
    {
        MSG_WARNING("The network manager for the AMX controller is not initialized!");
        return;
    }
}

void TPageManager::initOrientation()
{
    DECL_TRACER("TPageManager::initOrientation()");

    QAndroidJniObject activity = QtAndroid::androidActivity();
    QAndroidJniObject::callStaticMethod<void>("org/qtproject/theosys/Orientation", "Init", "(Landroid/app/Activity;)V", activity.object());
    activity.callStaticMethod<void>("org/qtproject/theosys/Orientation", "InstallOrientationListener", "()V");
}
#endif  // __ANDROID__

void TPageManager::setButtonCallbacks(Button::TButton *bt)
{
    bt->registerCallback(_displayButton);
    bt->regCallPlayVideo(_callPlayVideo);
    bt->setFonts(mFonts);
    bt->setPalette(mPalette);
}

void TPageManager::externalButton(extButtons_t bt, bool checked)
{
    DECL_TRACER("TPageManager::externalButton(extButtons_t bt)");

    if (!mExternal)
        return;

    EXTBUTTON_t button = mExternal->getButton(bt);

    if (button.type == EXT_NOBUTTON)
        return;

    if (button.cp && button.ch)
    {
        amx::ANET_SEND scmd;

        scmd.device = TConfig::getChannel();
        scmd.port = button.cp;
        scmd.channel = button.ch;

        if (checked)
            scmd.MC = 0x0084;   // push button
            else
                scmd.MC = 0x0085;   // release button

                MSG_DEBUG("Sending to device <" << scmd.device << ":" << scmd.port << ":0> channel " << scmd.channel << " value 0x" << std::setw(2) << std::setfill('0') << std::hex << scmd.MC << " (" << (checked?"PUSH":"RELEASE") << ")");

            if (gAmxNet)
                gAmxNet->sendCommand(scmd);
            else
            {
                MSG_WARNING("Missing global class TAmxNet. Can't send a message!");
            }
    }

}

void TPageManager::sendKeyboard(const std::string& text)
{
    DECL_TRACER("TPageManager::sendKeyboard(const std::string& text)");

    amx::ANET_SEND scmd;
    scmd.port = 1;
    scmd.channel = 0;
    scmd.msg = UTF8ToCp1250(text);
    scmd.MC = 0x008b;

    if (gAmxNet)
        gAmxNet->sendCommand(scmd);
    else
        MSG_WARNING("Missing global class TAmxNet. Can't send message!");
}

void TPageManager::sendKeypad(const std::string& text)
{
    DECL_TRACER("TPageManager::sendKeypad(const std::string& text)");

    amx::ANET_SEND scmd;
    scmd.port = 1;
    scmd.channel = 0;
    scmd.msg = UTF8ToCp1250(text);
    scmd.MC = 0x008b;

    if (gAmxNet)
        gAmxNet->sendCommand(scmd);
    else
        MSG_WARNING("Missing global class TAmxNet. Can't send message!");
}

void TPageManager::sendString(uint handle, const std::string& text)
{
    DECL_TRACER("TPageManager::sendString(uint handle, const std::string& text)");

    Button::TButton *bt = findButton(handle);

    if (!bt)
    {
        MSG_WARNING("Button " << TObject::handleToString(handle) << " not found!");
        return;
    }

    amx::ANET_SEND scmd;
    scmd.port = bt->getAddressPort();
    scmd.channel = bt->getAddressChannel();
    scmd.msg = UTF8ToCp1250(text);
    scmd.MC = 0x008b;

    if (gAmxNet)
        gAmxNet->sendCommand(scmd);
    else
        MSG_WARNING("Missing global class TAmxNet. Can't send message!");
}

void TPageManager::sendPHNcommand(const std::string& cmd)
{
    DECL_TRACER("TPageManager::sendPHNcommand(const std::string& cmd)");

    amx::ANET_SEND scmd;
    scmd.port = 1;
    scmd.channel = TConfig::getChannel();
    scmd.msg = "^PHN-" + cmd;
    scmd.MC = 0x008c;
    MSG_DEBUG("Sending PHN command: ^PHN-" << cmd);

    if (gAmxNet)
        gAmxNet->sendCommand(scmd);
    else
        MSG_WARNING("Missing global class TAmxNet. Can't send ^PHN command!");
}

void TPageManager::sendKeyStroke(char key)
{
    DECL_TRACER("TPageManager::sendKeyStroke(char key)");

    if (!key)
        return;

    char msg[2];
    msg[0] = key;
    msg[1] = 0;

    amx::ANET_SEND scmd;
    scmd.port = 1;
    scmd.channel = 0;
    scmd.msg.assign(msg);
    scmd.MC = 0x008c;

    if (gAmxNet)
        gAmxNet->sendCommand(scmd);
    else
        MSG_WARNING("Missing global class TAmxNet. Can't send message!");
}

/**
 * Sending a custom event is identical in all cases. Because of this I
 * implemented this method to send a custom event. This is called in all cases
 * where a ?XXX command is received.
 *
 * @param value1    The instance of the button.
 * @param value2    The value of a numeric request or the length of the string.
 * @param value3    Always 0
 * @param msg       In case of a string this contains the string.
 * @param evType    This is the event type, a number between 1001 and 1099.
 * @param cp        Channel port of button.
 * @param cn        Channel number. of button.
 *
 * @return If all parameters are valid it returns TRUE.
 */
bool TPageManager::sendCustomEvent(int value1, int value2, int value3, const string& msg, int evType, int cp, int cn)
{
    DECL_TRACER("TPageManager::sendCustomEvent(int value1, int value2, int value3, const string& msg, int evType)");

    if (value1 < 1)
        return false;

    amx::ANET_SEND scmd;
    scmd.port = cp;
    scmd.channel = cn;
    scmd.ID = scmd.channel;
    scmd.flag = 0;
    scmd.type = evType;
    scmd.value1 = value1;   // instance
    scmd.value2 = value2;
    scmd.value3 = value3;
    scmd.msg = msg;

    if (!msg.empty())
        scmd.dtype = 0x0001;// Char array

    scmd.MC = 0x008d;       // custom event

    if (gAmxNet)
        gAmxNet->sendCommand(scmd);
    else
        MSG_WARNING("Missing global class TAmxNet. Can't send message!");

    return true;
}
#ifndef _NOSIP_
string TPageManager::sipStateToString(TSIPClient::SIP_STATE_t s)
{
    DECL_TRACER("TPageManager::sipStateToString(TSIPClient::SIP_STATE_t s)");

    switch(s)
    {
        case TSIPClient::SIP_CONNECTED:     return "CONNECTED";
        case TSIPClient::SIP_DISCONNECTED:  return "DISCONNECTED";
        case TSIPClient::SIP_HOLD:          return "HOLD";
        case TSIPClient::SIP_RINGING:       return "RINGING";
        case TSIPClient::SIP_TRYING:        return "TRYING";

        default:
            return "IDLE";
    }

    return "IDLE";
}
#endif
/****************************************************************************
 * The following functions implements one of the commands the panel accepts.
 ****************************************************************************/

/**
 * This is a special function handling the progress bars when the files of the
 * panel are updated. Instead of simply displaying a ready page, it fakes one
 * with the actual dimensions of the main page. This is possible, because we've
 * always a main page even if the panel is started for the first time.
 */
void TPageManager::doFTR(int port, vector<int>& channels, vector<string>& pars)
{
    DECL_TRACER("TPageManager::doFTR(int, vector<int>&, vector<string>& pars)");

    if (pars.empty())
    {
        MSG_WARNING("Command #FTR needs at least 1 parameter! Ignoring command.");
        return;
    }

    if (TStreamError::checkFilter(HLOG_DEBUG))
    {
        for (size_t i = 0; i < pars.size(); i++)
        {
            MSG_DEBUG("[" << i << "]: " << pars.at(i));
        }
    }

    if (pars.at(0).compare("START") == 0)
    {
        // Here we have to drop all pages and subpages first and then display
        // the faked page with the progress bars.
        MSG_DEBUG("Starting file transfer ...");
        doPPX(port, channels, pars);
        TPage *pg = getPage("_progress");

        if (!pg)
        {
            if (!readPage("_progress"))
            {
                MSG_ERROR("Error creating the system page _progress!");
                return;
            }

            pg = getPage("_progress");

            if (!pg)
            {
                MSG_ERROR("Error getting system page _progress!");
                return;
            }
        }

        pg->setFonts(mFonts);
        pg->registerCallback(_setBackground);
        pg->regCallPlayVideo(_callPlayVideo);

        if (!pg || !_setPage || !mTSettings)
            return;

        int width, height;
        width = mTSettings->getWith();
        height = mTSettings->getHeight();
#ifdef _SCALE_SKIA_
        if (mScaleFactor != 1.0)
        {
            width = (int)((double)width * mScaleFactor);
            height = (int)((double)height * mScaleFactor);
        }
#endif
        _setPage((pg->getNumber() << 16) & 0xffff0000, width, height);
        pg->show();
        MSG_DEBUG("Page _progress on screen");
    }
    else if (pars.at(0).compare("SYNC") == 0)
    {
        TPage *pg = getPage("_progress");

        if (!pg)
        {
            MSG_ERROR("Page _progress not found!");
            return;
        }

        Button::TButton *bt = pg->getButton(1);   // Line 1

        if (!bt)
        {
            MSG_ERROR("Button 160 of page _progress not found!");
            return;
        }

        bt->setText(pars.at(2), 0);
        bt->show();
    }
    else if (pars.at(0).compare("FTRSTART") == 0)
    {
        TPage *pg = getPage("_progress");

        if (!pg)
        {
            MSG_ERROR("Page _progress not found!");
            return;
        }

        Button::TButton *bt1 = pg->getButton(1);   // Line 1
        Button::TButton *bt2 = pg->getButton(2);   // Line 2
        Button::TButton *bt3 = pg->getButton(3);   // Bargraph 1
        Button::TButton *bt4 = pg->getButton(4);   // Bargraph 2

        if (!bt1 || !bt2 || !bt3 || !bt4)
        {
            MSG_ERROR("Buttons of page _progress not found!");
            return;
        }

        bt1->setText("Transfering files ...", 0);
        bt1->show();
        bt2->setText(pars.at(3), 0);
        bt2->show();
        bt3->drawBargraph(0, atoi(pars.at(1).c_str()), true);
        bt4->drawBargraph(0, atoi(pars.at(2).c_str()), true);
    }
    else if (pars.at(0).compare("FTRPART") == 0)
    {
        TPage *pg = getPage("_progress");

        if (!pg)
        {
            MSG_ERROR("Page _progress not found!");
            return;
        }

        Button::TButton *bt = pg->getButton(4);   // Bargraph 2

        if (!bt)
        {
            MSG_ERROR("Buttons of page _progress not found!");
            return;
        }

        bt->drawBargraph(0, atoi(pars.at(2).c_str()), true);
    }
    else if (pars.at(0).compare("END") == 0)
    {
        MSG_TRACE("End of file transfer reached.");

        if (_resetSurface)
            _resetSurface();
        else
        {
            MSG_WARNING("Missing callback function \"resetSurface\"!");
        }
    }
}

void TPageManager::doON(int port, vector<int>&, vector<string>& pars)
{
    DECL_TRACER("TPageManager::doON(int port, vector<int>& channels, vector<string>& pars)");

    if (pars.empty())
    {
        MSG_WARNING("Command ON needs 1 parameter! Ignoring command.");
        return;
    }

    TError::clear();
    int c = atoi(pars[0].c_str());

    if (c <= 0)
    {
        MSG_WARNING("Invalid channel " << c << "! Ignoring command.");
        return;
    }

    vector<int> chans = { c };
    vector<MAP_T> map = findButtons(port, chans, TYPE_CM);

    if (TError::isError() || map.empty())
        return;

    vector<Button::TButton *> buttons = collectButtons(map);

    if (buttons.size() > 0)
    {
        vector<Button::TButton *>::iterator mapIter;

        for (mapIter = buttons.begin(); mapIter != buttons.end(); ++mapIter)
        {
            Button::TButton *bt = *mapIter;

            if (bt->getButtonType() == Button::GENERAL)
                bt->setActive(1);
        }
    }
}

void TPageManager::doOFF(int port, vector<int>&, vector<string>& pars)
{
    DECL_TRACER("TPageManager::doOFF(int port, vector<int>& channels, vector<string>& pars)");

    if (pars.empty())
    {
        MSG_WARNING("Command OFF needs 1 parameter! Ignoring command.");
        return;
    }

    TError::clear();
    int c = atoi(pars[0].c_str());

    if (c <= 0)
    {
        MSG_WARNING("Invalid channel " << c << "! Ignoring command.");
        return;
    }

    vector<int> chans = { c };
    vector<MAP_T> map = findButtons(port, chans, TYPE_CM);

    if (TError::isError() || map.empty())
        return;

    vector<Button::TButton *> buttons = collectButtons(map);

    if (buttons.size() > 0)
    {
        vector<Button::TButton *>::iterator mapIter;

        for (mapIter = buttons.begin(); mapIter != buttons.end(); ++mapIter)
        {
            Button::TButton *bt = *mapIter;

            if (bt->getButtonType() == Button::GENERAL)
                bt->setActive(0);
        }
    }
}

void TPageManager::doLEVEL(int port, vector<int>&, vector<string>& pars)
{
    DECL_TRACER("TPageManager::doLEVEL(int port, vector<int>& channels, vector<string>& pars)");

    if (pars.size() < 2)
    {
        MSG_WARNING("Command LEVEL needs 2 parameters! Ignoring command.");
        return;
    }

    TError::clear();
    int c = atoi(pars[0].c_str());
    int level = atoi(pars[1].c_str());

    if (c <= 0)
    {
        MSG_WARNING("Invalid channel " << c << "! Ignoring command.");
        return;
    }

    vector<int> chans = { c };
    vector<MAP_T> map = findBargraphs(port, chans);

    if (TError::isError() || map.empty())
    {
        MSG_WARNING("No bargraphs found!");
        return;
    }

    vector<Button::TButton *> buttons = collectButtons(map);

    if (buttons.size() > 0)
    {
        vector<Button::TButton *>::iterator mapIter;

        for (mapIter = buttons.begin(); mapIter != buttons.end(); ++mapIter)
        {
            Button::TButton *bt = *mapIter;

            if (bt->getButtonType() == Button::BARGRAPH)
                bt->drawBargraph(bt->getActiveInstance(), level);
            else if (bt->getButtonType() == Button::MULTISTATE_BARGRAPH)
            {
                int state = (int)((double)bt->getStateCount() / (double)(bt->getRangeHigh() - bt->getRangeLow()) * (double)level);
                bt->setActive(state);
            }
        }
    }
}

void TPageManager::doBLINK(int, vector<int>&, vector<string>& pars)
{
    DECL_TRACER("TPageManager::doBLINK(int port, vector<int>& channels, vector<string>& pars)");

    if (pars.size() < 4)
    {
        MSG_WARNING("Command BLINK expects 4 parameters! Command ignored.");
        return;
    }

    TError::clear();
    vector<int> sysButtons = { 141, 142, 143, 151, 152, 153, 154, 155, 156, 157, 158 };
    vector<MAP_T> map = findButtons(0, sysButtons);

    if (TError::isError() || map.empty())
    {
        MSG_WARNING("No system buttons found.");
        return;
    }

    vector<Button::TButton *> buttons = collectButtons(map);
    vector<Button::TButton *>::iterator mapIter;

    for (mapIter = buttons.begin(); mapIter != buttons.end(); mapIter++)
    {
        Button::TButton *bt = *mapIter;
        bt->setActive(0);
    }
}

void TPageManager::doVER(int, vector<int>&, vector<string>&)
{
    DECL_TRACER("TPageManager::doVER(int, vector<int>&, vector<string>&)");

    amx::ANET_SEND scmd;
    scmd.port = 1;
    scmd.channel = 0;
    scmd.msg.assign(string("^VER-")+VERSION_STRING());
    scmd.MC = 0x008c;

    if (gAmxNet)
        gAmxNet->sendCommand(scmd);
    else
        MSG_WARNING("Missing global class TAmxNet. Can't send message!");
}

void TPageManager::doWCN(int, vector<int>&, vector<string>&)
{
    DECL_TRACER("TPageManager::doWCN(int, vector<int>&, vector<string>&)");

    if (!TConfig::getSIPstatus())
        return;

    amx::ANET_SEND scmd;
    scmd.port = 1;
    scmd.channel = 0;
    scmd.msg.assign("^WCN-" + TConfig::getSIPuser());
    scmd.MC = 0x008c;

    if (gAmxNet)
        gAmxNet->sendCommand(scmd);
    else
        MSG_WARNING("Missing global class TAmxNet. Can't send message!");
}

/**
 * Add a specific popup page to a specified popup group if it does not already
 * exist. If the new popup is added to a group which has a popup displayed on
 * the current page along with the new pop-up, the displayed popup will be
 * hidden and the new popup will be displayed.
 */
void TPageManager::doAPG(int, std::vector<int>&, std::vector<std::string>& pars)
{
    DECL_TRACER("TPageManager::doAPG(int port, std::vector<int>& channels, std::vector<std::string>& pars)");

    if (pars.size() < 2)
    {
        MSG_ERROR("Less than 2 parameters!");
        return;
    }

    TError::clear();
    closeGroup(pars[1]);

    TPage *page = nullptr;
    TSubPage *subPage = deliverSubPage(pars[0], &page);

    if (!subPage)
    {
        MSG_ERROR("Subpage " << pars[0] << " couldn't either found or created!");
        return;
    }

    subPage->setGroup(pars[1]);
    subPage->setZOrder(page->getNextZOrder());
    subPage->show();
}

/**
 * Clear all popup pages from specified popup group.
 */
void TPageManager::doCPG(int, std::vector<int>&, std::vector<std::string>& pars)
{
    DECL_TRACER("TPageManager::doCPG(int port, std::vector<int>& channels, std::vector<std::string>& pars)");

    if (pars.size() < 1)
    {
        MSG_ERROR("Expecting 1 parameter but got only 1!");
        return;
    }

    TError::clear();
    vector<SUBPAGELIST_T> pageList = mPageList->getSupPageList();

    if (pageList.size() > 0)
    {
        vector<SUBPAGELIST_T>::iterator pgIter;

        for (pgIter = pageList.begin(); pgIter != pageList.end(); pgIter++)
        {
            if (pgIter->group.compare(pars[0]) == 0)
            {
                pgIter->group.clear();
                TSubPage *pg = getSubPage(pgIter->pageID);

                if (pg)
                    pg->setGroup(pgIter->group);
            }
        }
    }
}

/**
 * Delete a specific popup page from specified popup group if it exists.
 */
void TPageManager::doDPG(int, std::vector<int>&, std::vector<std::string>& pars)
{
    DECL_TRACER("TPageManager::doDPG(int port, std::vector<int>& channels, std::vector<std::string>& pars)");

    if (pars.size() < 2)
    {
        MSG_ERROR("Less than 2 parameters!");
        return;
    }

    TError::clear();
    SUBPAGELIST_T listPg = findSubPage(pars[0]);

    if (!listPg.isValid)
        return;

    if (listPg.group.compare(pars[1]) == 0)
    {
        listPg.group.clear();
        TSubPage *pg = getSubPage(listPg.pageID);

        if (pg)
            pg->setGroup(listPg.group);
    }
}

/**
 * Set the hide effect for the specified popup page to the named hide effect.
 */
void TPageManager::doPHE(int, vector<int>&, vector<string>& pars)
{
    DECL_TRACER("TPageManager::doPHE(int port, vector<int>& channels, vector<string>& pars)");

    if (pars.size() < 2)
    {
        MSG_ERROR("Less than 2 parameters!");
        return;
    }

    TError::clear();
    TSubPage *pg = deliverSubPage(pars[0]);

    if (!pg)
        return;

    if (pars[1].compare("fade") == 0)
        pg->setHideEffect(SE_FADE);
    else if (pars[1].compare("slide to left") == 0)
        pg->setHideEffect(SE_SLIDE_LEFT);
    else if (pars[1].compare("slide to right") == 0)
        pg->setHideEffect(SE_SLIDE_RIGHT);
    else if (pars[1].compare("slide to top") == 0)
        pg->setHideEffect(SE_SLIDE_TOP);
    else if (pars[1].compare("slide to bottom") == 0)
        pg->setHideEffect(SE_SLIDE_BOTTOM);
    else if (pars[1].compare("slide to left fade") == 0)
        pg->setHideEffect(SE_SLIDE_LEFT_FADE);
    else if (pars[1].compare("slide to right fade") == 0)
        pg->setHideEffect(SE_SLIDE_RIGHT_FADE);
    else if (pars[1].compare("slide to top fade") == 0)
        pg->setHideEffect(SE_SLIDE_TOP_FADE);
    else if (pars[1].compare("slide to bottom fade") == 0)
        pg->setHideEffect(SE_SLIDE_BOTTOM_FADE);
    else
        pg->setHideEffect(SE_NONE);
}

/**
 * Set the hide effect position. Only 1 coordinate is ever needed for an effect;
 * however, the command will specify both. This command sets the location at
 * which the effect will end at.
 */
void TPageManager::doPHP(int, vector<int>&, vector<string>& pars)
{
    DECL_TRACER("TPageManager::doPHP(int port, vector<int>& channels, vector<string>& pars)");

    if (pars.size() < 2)
    {
        MSG_ERROR("Less than 2 parameters!");
        return;
    }

    TError::clear();
    size_t pos = pars[1].find(",");
    int x, y;

    if (pos == string::npos)
    {
        x = atoi(pars[1].c_str());
        y = 0;
    }
    else
    {
        x = atoi(pars[1].substr(0, pos).c_str());
        y = atoi(pars[1].substr(pos+1).c_str());
    }

    TSubPage *pg = deliverSubPage(pars[0]);

    if (!pg)
        return;

    pg->setHideEndPosition(x, y);
}

/**
 * Set the hide effect time for the specified popup page.
 */
void TPageManager::doPHT(int, vector<int>&, vector<string>& pars)
{
    DECL_TRACER("TPageManager::doPHT(int port, vector<int>& channels, vector<string>& pars)");

    if (pars.size() < 2)
    {
        MSG_ERROR("Less than 2 parameters!");
        return;
    }

    TError::clear();
    TSubPage *pg = deliverSubPage(pars[0]);

    if (!pg)
        return;

    pg->setHideTime(atoi(pars[1].c_str()));
}

/**
 * Close all popups on a specified page. If the page name is empty, the current
 * page is used. Same as the ’Clear Page’ command in TPDesign4.
 */
void TPageManager::doPPA(int, std::vector<int>&, std::vector<std::string>& pars)
{
    DECL_TRACER("TPageManager::doPPA(int port, std::vector<int>& channels, std::vector<std::string>& pars)");

    TError::clear();
    TPage *pg;

    if (pars.size() == 0)
        pg = getPage(mActualPage);
    else
        pg = getPage(pars[0]);

    if (!pg)
        return;

    pg->drop();
    pg->resetZOrder();
}

/**
 * Deactivate a specific popup page on either a specified page or the current
 * page. If the page name is empty, the current page is used. If the popup page
 * is part of a group, the whole group is deactivated. This command works in
 * the same way as the ’Hide Popup’ command in TPDesign4.
 */
void TPageManager::doPPF(int, std::vector<int>&, std::vector<std::string>& pars)
{
    DECL_TRACER("TPageManager::doPPF(int port, std::vector<int>& channels, std::vector<std::string>& pars)");

    if (pars.size() < 1)
    {
        MSG_ERROR("At least 1 parameter is expected!");
        return;
    }

    TError::clear();
    hideSubPage(pars[0]);
}

/**
 * Toggle a specific popup page on either a specified page or the current page.
 * If the page name is empty, the current page is used. Toggling refers to the
 * activating/deactivating (On/Off) of a popup page. This command works in the
 * same way as the ’Toggle Popup’ command in TPDesign4.
 */
void TPageManager::doPPG(int, std::vector<int>&, std::vector<std::string>& pars)
{
    DECL_TRACER("TPageManager::doPPG(int port, std::vector<int>& channels, std::vector<std::string>& pars)");

    if (pars.size() < 1)
    {
        MSG_ERROR("At least 1 parameter is expected!");
        return;
    }

    TError::clear();
    TPage *page = getPage(mActualPage);

    if (!page)
    {
        MSG_ERROR("No active page found! Internal error.");
        return;
    }

    TSubPage *pg = getSubPage(pars[0]);

    if (!pg)
        return;

    if (pg->isVisible())
    {
        if (pg->getZOrder() == page->getActZOrder())
            page->decZOrder();

        pg->drop();
        return;
    }

    TSubPage *sub = getFirstSubPageGroup(pg->getGroupName());

    while(sub)
    {
        if (sub->getGroupName().compare(pg->getGroupName()) == 0 && sub->isVisible())
            sub->drop();

        sub = getNextSubPageGroup(pg->getGroupName(), sub);
    }

    pg->show();
    pg->setZOrder(page->getNextZOrder());
}

/**
 * Kill refers to the deactivating (Off) of a popup window from all pages. If
 * the pop-up page is part of a group, the whole group is deactivated. This
 * command works in the same way as the 'Clear Group' command in TPDesign 4.
 */
void TPageManager::doPPK(int, std::vector<int>&, std::vector<std::string>& pars)
{
    DECL_TRACER("TPageManager::doPPK(int port, std::vector<int>& channels, std::vector<std::string>& pars)");

    if (pars.size() < 1)
    {
        MSG_ERROR("At least 1 parameter is expected!");
        return;
    }

    TError::clear();
    TPage *page = getPage(mActualPage);

    if (!page)
    {
        MSG_ERROR("No active page found! Internal error.");
        return;
    }

    TSubPage *pg = getSubPage(pars[0]);

    if (pg)
    {
        if (pg->getZOrder() == page->getActZOrder())
            page->decZOrder();

        pg->drop();
    }
}

/**
 * Set the modality of a specific popup page to Modal or NonModal.
 * A Modal popup page, when active, only allows you to use the buttons and
 * features on that popup page. All other buttons on the panel page are
 * inactivated.
 */
void TPageManager::doPPM(int, std::vector<int>&, std::vector<std::string>& pars)
{
    DECL_TRACER("TPageManager::doPPM(int port, std::vector<int>& channels, std::vector<std::string>& pars)");

    if (pars.size() < 2)
    {
        MSG_ERROR("Expecting 2 parameters!");
        return;
    }

    TError::clear();
    TSubPage *pg = getSubPage(pars[0]);

    if (pg)
    {
        if (pars[1] == "1" || pars[1].compare("modal") == 0 || pars[1].compare("MODAL") == 0)
            pg->setModal(1);
        else
            pg->setModal(0);
    }
}

/**
 * Activate a specific popup page to launch on either a specified page or the
 * current page. If the page name is empty, the current page is used. If the
 * popup page is already on, do not re-draw it. This command works in the same
 * way as the ’Show Popup’ command in TPDesign4.
 */
void TPageManager::doPPN(int, std::vector<int>&, std::vector<std::string>& pars)
{
    DECL_TRACER("TPageManager::doPPN(int port, std::vector<int>& channels, std::vector<std::string>& pars)");

    if (pars.size() < 1)
    {
        MSG_ERROR("At least 1 parameter is expected!");
        return;
    }

    TError::clear();
    showSubPage(pars[0]);
}

/**
 * Set a specific popup page to timeout within a specified time. If timeout is
 * empty, popup page will clear the timeout.
 */
void TPageManager::doPPT(int, vector<int>&, vector<string>& pars)
{
    DECL_TRACER("TPageManager::doPPT(int port, vector<int>& channels, vector<string>& pars)");

    if (pars.size() < 2)
    {
        MSG_ERROR("Expecting 2 parameters!");
        return;
    }

    TError::clear();
    TSubPage *pg = deliverSubPage(pars[0]);

    if (!pg)
        return;

    pg->setTimeout(atoi(pars[1].c_str()));
}

/**
 * Close all popups on all pages. This command works in the same way as the
 * 'Clear All' command in TPDesign 4.
 */
void TPageManager::doPPX(int, std::vector<int>&, std::vector<std::string>&)
{
    DECL_TRACER("TPageManager::doPPX(int port, std::vector<int>& channels, std::vector<std::string>& pars)");

    TError::clear();
    PCHAIN_T *chain = mPchain;

    while(chain)
    {
        TSubPage *sub = chain->page->getFirstSubPage();

        while (sub)
        {
            MSG_DEBUG("Dopping subpage " << sub->getNumber() << ", \"" << sub->getName() << "\".");
            sub->drop();
            sub = chain->page->getNextSubPage();
        }

        chain = chain->next;
    }

    TPage *page = getPage(mActualPage);

    if (!page)
    {
        MSG_ERROR("No active page found! Internal error.");
        return;
    }

    page->resetZOrder();
}

/**
 * Set the show effect for the specified popup page to the named show effect.
 */
void TPageManager::doPSE(int, vector<int>&, vector<string>& pars)
{
    DECL_TRACER("TPageManager::doPSE(int port, vector<int>& channels, vector<string>& pars)");

    if (pars.size() < 2)
    {
        MSG_ERROR("Less than 2 parameters!");
        return;
    }

    TError::clear();
    TSubPage *pg = deliverSubPage(pars[0]);

    if (!pg)
        return;

    if (pars[1].compare("fade") == 0)
        pg->setShowEffect(SE_FADE);
    else if (pars[1].compare("slide to left") == 0)
        pg->setShowEffect(SE_SLIDE_LEFT);
    else if (pars[1].compare("slide to right") == 0)
        pg->setShowEffect(SE_SLIDE_RIGHT);
    else if (pars[1].compare("slide to top") == 0)
        pg->setShowEffect(SE_SLIDE_TOP);
    else if (pars[1].compare("slide to bottom") == 0)
        pg->setShowEffect(SE_SLIDE_BOTTOM);
    else if (pars[1].compare("slide to left fade") == 0)
        pg->setShowEffect(SE_SLIDE_LEFT_FADE);
    else if (pars[1].compare("slide to right fade") == 0)
        pg->setShowEffect(SE_SLIDE_RIGHT_FADE);
    else if (pars[1].compare("slide to top fade") == 0)
        pg->setShowEffect(SE_SLIDE_TOP_FADE);
    else if (pars[1].compare("slide to bottom fade") == 0)
        pg->setShowEffect(SE_SLIDE_BOTTOM_FADE);
    else
        pg->setShowEffect(SE_NONE);
}

void TPageManager::doPSP(int, vector<int>&, vector<string>& pars)
{
    DECL_TRACER("TPageManager::doPSP(int port, vector<int>& channels, vector<string>& pars)");

    if (pars.size() < 2)
    {
        MSG_ERROR("Less than 2 parameters!");
        return;
    }

    TError::clear();
    size_t pos = pars[1].find(",");
    int x, y;

    if (pos == string::npos)
    {
        x = atoi(pars[1].c_str());
        y = 0;
    }
    else
    {
        x = atoi(pars[1].substr(0, pos).c_str());
        y = atoi(pars[1].substr(pos+1).c_str());
    }

    TSubPage *pg = deliverSubPage(pars[0]);

    if (!pg)
        return;

    pg->setShowEndPosition(x, y);
}

/**
 * Set the show effect time for the specified popup page.
 */
void TPageManager::doPST(int, vector<int>&, vector<string>& pars)
{
    DECL_TRACER("TPageManager::doPST(int port, vector<int>& channels, vector<string>& pars)");

    if (pars.size() < 2)
    {
        MSG_ERROR("Less than 2 parameters!");
        return;
    }

    TError::clear();
    TSubPage *pg = deliverSubPage(pars[0]);

    if (!pg)
        return;

    pg->setShowTime(atoi(pars[1].c_str()));
}

/**
 * Flips to a page with a specified page name. If the page is currently active,
 * it will not redraw the page.
 */
void TPageManager::doPAGE(int, std::vector<int>&, std::vector<std::string>& pars)
{
    DECL_TRACER("TPageManager::doPAGE(int port, std::vector<int>& channels, std::vector<std::string>& pars)");

    if (pars.empty())
    {
        MSG_WARNING("Got no page parameter!");
        return;
    }

    TError::clear();
    setPage(pars[0]);
}

/**
 * @brief TPageManager::doANI Run a button animation (in 1/10 second).
 * Syntax:
 *      ^ANI-<vt addr range>,<start state>,<end state>,<time>
 * Variable:
 *      variable text address range = 1 - 4000.
 *      start state = Beginning of button state (0= current state).
 *      end state = End of button state.
 *      time = In 1/10 second intervals.
 * Example:
 *      SEND_COMMAND Panel,"'^ANI-500,1,25,100'"
 * Runs a button animation at text range 500 from state 1 to state 25 for 10 seconds.
 *
 * @param port      The port number
 * @param channels  The channels of the buttons
 * @param pars      The parameters
 */
void TPageManager::doANI(int port, std::vector<int> &channels, std::vector<std::string> &pars)
{
    DECL_TRACER("TPageManager::doANI(int port, std::vector<int> &channels, std::vector<std::string> &pars)");

    if (pars.size() < 3)
    {
        MSG_ERROR("Expecting 3 parameters but got " << pars.size() << "! Ignoring command.");
        return;
    }

    TError::clear();
    int stateStart = atoi(pars[0].c_str());
    int endState = atoi(pars[1].c_str());
    int runTime = atoi(pars[2].c_str());

    vector<MAP_T> map = findButtons(port, channels);

    if (TError::isError() || map.empty())
        return;

    vector<Button::TButton *> buttons = collectButtons(map);

    if (buttons.size() > 0)
    {
        vector<Button::TButton *>::iterator mapIter;

        for (mapIter = buttons.begin(); mapIter != buttons.end(); mapIter++)
        {
            Button::TButton *bt = *mapIter;
            bt->startAnimation(stateStart, endState, runTime);
        }
    }
}

/**
 * Add page flip action to a button if it does not already exist.
 */
void TPageManager::doAPF(int port, vector<int>& channels, vector<string>& pars)
{
    DECL_TRACER("TPageManager::doAPF(int port, vector<int>& channels, vector<string>& pars)");

    if (pars.size() < 2)
    {
        MSG_ERROR("Expecting 2 parameters but got " << pars.size() << "! Ignoring command.");
        return;
    }

    TError::clear();
    string action = pars[0];
    string pname = pars[1];

    vector<MAP_T> map = findButtons(port, channels);

    if (TError::isError() || map.empty())
        return;

    vector<Button::TButton *> buttons = collectButtons(map);

    if (buttons.size() > 0)
    {
        vector<Button::TButton *>::iterator mapIter;

        for (mapIter = buttons.begin(); mapIter != buttons.end(); mapIter++)
        {
            Button::TButton *bt = *mapIter;
            setButtonCallbacks(bt);
            bt->addPushFunction(action, pname);
        }
    }
}

/**
 * Append non-unicode text.
 */
void TPageManager::doBAT(int port, vector<int> &channels, vector<string> &pars)
{
    DECL_TRACER("TPageManager::doBAT(int port, vector<int> &channels, vector<string> &pars)");

    if (pars.size() < 1)
    {
        MSG_ERROR("Expecting 1 parameters but got none! Ignoring command.");
        return;
    }

    TError::clear();
    int btState = atoi(pars[0].c_str());
    string text;

    if (pars.size() > 1)
        text = pars[1];

    vector<MAP_T> map = findButtons(port, channels);

    if (TError::isError() || map.empty())
        return;

    vector<Button::TButton *> buttons = collectButtons(map);

    if (buttons.size() > 0)
    {
        vector<Button::TButton *>::iterator mapIter;

        for (mapIter = buttons.begin(); mapIter != buttons.end(); mapIter++)
        {
            Button::TButton *bt = *mapIter;
            setButtonCallbacks(bt);

            if (btState == 0)       // All instances?
            {
                int bst = bt->getNumberInstances();

                for (int i = 0; i < bst; i++)
                    bt->appendText(text, i);
            }
            else
                bt->appendText(text, btState - 1);
        }
    }
}

/**
 * @brief Append unicode text. Same format as ^UNI.
 * This command allows to set up to 50 characters of ASCII code. The unicode
 * characters must be set as hex numbers.
 */
void TPageManager::doBAU(int port, vector<int>& channels, vector<string>& pars)
{
    DECL_TRACER("TPageManager::doBAU(int port, vector<int>& channels, vector<string>& pars)");

    if (pars.size() < 1)
    {
        MSG_ERROR("Expecting 1 parameters but got none! Ignoring command.");
        return;
    }

    TError::clear();
    int btState = atoi(pars[0].c_str());
    string text;
    char ch[3];

    if (pars.size() > 1)
    {
        text = pars[1];
        // Because the unicode characters are hex numbers, we scan the text
        // and convert the hex numbers into real numbers.
        size_t len = text.length();
        string t;
        bool inHex = false;
        int lastChar = 0;

        for (size_t i = 0; i < len; i++)
        {
            if (!inHex && isHex(text.at(i)))
            {
                inHex = true;
                lastChar = text.at(i);
                continue;
            }

            if (inHex && !isHex(text.at(i)))
            {
                inHex = false;
                ch[0] = lastChar;
                ch[1] = 0;
                t.append(ch);
            }

            if (inHex && isHex(text.at(i)))
            {
                ch[0] = lastChar;
                ch[1] = text.at(i);
                ch[2] = 0;
                int num = (int)strtol(ch, NULL, 16);
                ch[0] = num;
                ch[1] = 0;
                t.append(ch);
                inHex = false;
                continue;
            }

            ch[0] = text.at(i);
            ch[1] = 0;
            t.append(ch);
        }

        text = t;
    }

    vector<MAP_T> map = findButtons(port, channels);

    if (TError::isError() || map.empty())
        return;

    vector<Button::TButton *> buttons = collectButtons(map);

    if (buttons.size() > 0)
    {
        vector<Button::TButton *>::iterator mapIter;

        for (mapIter = buttons.begin(); mapIter != buttons.end(); mapIter++)
        {
            Button::TButton *bt = *mapIter;
            setButtonCallbacks(bt);

            if (btState == 0)       // All instances?
            {
                int bst = bt->getNumberInstances();

                for (int i = 0; i < bst; i++)
                    bt->appendText(text, i);
            }
            else
                bt->appendText(text, btState - 1);
        }
    }
}

/**
 * @brief TPageManager::doBCB Set the border color.
 * Set the border color to the specified color. Only if the specified border
 * color is not the same as the current color.
 * Note: Color can be assigned by color name (without spaces), number or
 * R,G,B value (RRGGBB or RRGGBBAA).
 */
void TPageManager::doBCB(int port, vector<int> &channels, vector<string> &pars)
{
    DECL_TRACER("TPageManager::doBCB(int port, vector<int> &channels, vector<string> &pars)");

    if (pars.size() < 1)
    {
        MSG_ERROR("Expecting 1 parameters but got none! Ignoring command.");
        return;
    }

    TError::clear();
    int btState = atoi(pars[0].c_str());
    string color;

    if (pars.size() > 1)
        color = pars[1];

    vector<MAP_T> map = findButtons(port, channels);

    if (TError::isError() || map.empty())
        return;

    vector<Button::TButton *> buttons = collectButtons(map);

    if (buttons.size() > 0)
    {
        vector<Button::TButton *>::iterator mapIter;

        for (mapIter = buttons.begin(); mapIter != buttons.end(); mapIter++)
        {
            Button::TButton *bt = *mapIter;
            setButtonCallbacks(bt);

            if (btState == 0)       // All instances?
            {
                int bst = bt->getNumberInstances();

                for (int i = 0; i < bst; i++)
                    bt->setBorderColor(color, i);
            }
            else
                bt->setBorderColor(color, btState - 1);
        }
    }
}

void TPageManager::getBCB(int port, vector<int> &channels, vector<string> &pars)
{
    DECL_TRACER("TPageManager::getBCB(int port, vector<int> &channels, vector<string> &pars)");

    if (pars.size() < 1)
    {
        MSG_ERROR("Expecting 1 parameters but got none! Ignoring command.");
        return;
    }

    TError::clear();
    int btState = atoi(pars[0].c_str());
    string color;

    vector<MAP_T> map = findButtons(port, channels);

    if (TError::isError() || map.empty())
        return;

    vector<Button::TButton *> buttons = collectButtons(map);

    if (buttons.size() > 0)
    {
        Button::TButton *bt = buttons[0];

        if (btState == 0)       // All instances?
        {
            int bst = bt->getNumberInstances();

            for (int i = 0; i < bst; i++)
            {
                color = bt->getBorderColor(i);

                if (color.empty())
                    continue;

                sendCustomEvent(i + 1, color.length(), 0, color, 1011, bt->getChannelPort(), bt->getChannelNumber());
            }
        }
        else
        {
            color = bt->getBorderColor(btState - 1);

            if (color.empty())
                return;

            sendCustomEvent(btState, color.length(), 0, color, 1011, bt->getChannelPort(), bt->getChannelNumber());
        }
    }
}

/**
 * @brief Set the fill color to the specified color.
 * Only if the specified fill color is not the same as the current color.
 * Note: Color can be assigned by color name (without spaces), number or R,G,B value (RRGGBB or RRGGBBAA).
 */
void TPageManager::doBCF(int port, vector<int>& channels, vector<std::string>& pars)
{
    DECL_TRACER("TPageManager::doBCF(int port, vector<int>& channels, vector<std::string>& pars)");

    if (pars.size() < 1)
    {
        MSG_ERROR("Expecting 1 parameters but got none! Ignoring command.");
        return;
    }

    TError::clear();
    int btState = atoi(pars[0].c_str());
    string color;

    if (pars.size() > 1)
        color = pars[1];

    vector<MAP_T> map = findButtons(port, channels);

    if (TError::isError() || map.empty())
        return;

    vector<Button::TButton *> buttons = collectButtons(map);

    if (buttons.size() > 0)
    {
        vector<Button::TButton *>::iterator mapIter;

        for (mapIter = buttons.begin(); mapIter != buttons.end(); mapIter++)
        {
            Button::TButton *bt = *mapIter;
            setButtonCallbacks(bt);

            if (btState == 0)       // All instances?
            {
                int bst = bt->getNumberInstances();

                for (int i = 0; i < bst; i++)
                    bt->setFillColor(color, i);
            }
            else
                bt->setFillColor(color, btState - 1);
        }
    }
}

void TPageManager::getBCF(int port, vector<int> &channels, vector<string> &pars)
{
    DECL_TRACER("TPageManager::getBCF(int port, vector<int> &channels, vector<string> &pars)");

    if (pars.size() < 1)
    {
        MSG_ERROR("Expecting 1 parameters but got none! Ignoring command.");
        return;
    }

    TError::clear();
    int btState = atoi(pars[0].c_str());
    string color;

    vector<MAP_T> map = findButtons(port, channels);

    if (TError::isError() || map.empty())
        return;

    vector<Button::TButton *> buttons = collectButtons(map);

    if (buttons.size() > 0)
    {
        Button::TButton *bt = buttons[0];

        if (btState == 0)       // All instances?
        {
            int bst = bt->getNumberInstances();

            for (int i = 0; i < bst; i++)
            {
                color = bt->getFillColor(i);

                if (color.empty())
                    continue;

                sendCustomEvent(i + 1, color.length(), 0, color, 1012, bt->getChannelPort(), bt->getChannelNumber());
            }
        }
        else
        {
            color = bt->getFillColor(btState-1);
            sendCustomEvent(btState, color.length(), 0, color, 1012, bt->getChannelPort(), bt->getChannelNumber());
        }
    }
}

/**
 * @brief Set the text color to the specified color.
 * Only if the specified text color is not the same as the current color.
 * Note: Color can be assigned by color name (without spaces), number or R,G,B value (RRGGBB or RRGGBBAA).
 */
void TPageManager::doBCT(int port, vector<int>& channels, vector<string>& pars)
{
    DECL_TRACER("TPageManager::doBCT(int port, vector<int>& channels, vector<string>& pars)");

    if (pars.size() < 1)
    {
        MSG_ERROR("Expecting 1 parameters but got none! Ignoring command.");
        return;
    }

    TError::clear();
    int btState = atoi(pars[0].c_str());
    string color;

    if (pars.size() > 1)
        color = pars[1];

    vector<MAP_T> map = findButtons(port, channels);

    if (TError::isError() || map.empty())
        return;

    vector<Button::TButton *> buttons = collectButtons(map);

    if (buttons.size() > 0)
    {
        vector<Button::TButton *>::iterator mapIter;

        for (mapIter = buttons.begin(); mapIter != buttons.end(); mapIter++)
        {
            Button::TButton *bt = *mapIter;
            setButtonCallbacks(bt);

            if (btState == 0)       // All instances?
            {
                int bst = bt->getNumberInstances();

                for (int i = 0; i < bst; i++)
                    bt->setTextColor(color, i);
            }
            else
                bt->setTextColor(color, btState - 1);
        }
    }
}

void TPageManager::getBCT(int port, vector<int> &channels, vector<string> &pars)
{
    DECL_TRACER("TPageManager::getBCT(int port, vector<int> &channels, vector<string> &pars)");

    if (pars.size() < 1)
    {
        MSG_ERROR("Expecting 1 parameters but got none! Ignoring command.");
        return;
    }

    TError::clear();
    int btState = atoi(pars[0].c_str());
    string color;

    vector<MAP_T> map = findButtons(port, channels);

    if (TError::isError() || map.empty())
        return;

    vector<Button::TButton *> buttons = collectButtons(map);

    if (buttons.size() > 0)
    {
        Button::TButton *bt = buttons[0];

        if (btState == 0)       // All instances?
        {
            int bst = bt->getNumberInstances();

            for (int i = 0; i < bst; i++)
            {
                color = bt->getTextColor(i);

                if (color.empty())
                    continue;

                sendCustomEvent(i + 1, color.length(), 0, color, 1013, bt->getChannelPort(), bt->getChannelNumber());
            }
        }
        else
        {
            color = bt->getTextColor(btState - 1);
            sendCustomEvent(btState, color.length(), 0, color, 1013, bt->getChannelPort(), bt->getChannelNumber());
        }
    }
}

/**
 * Set the button draw order
 * Determines what order each layer of the button is drawn.
 */
void TPageManager::doBDO(int port, vector<int>& channels, vector<std::string>& pars)
{
    DECL_TRACER("TPageManager::doBDO(int port, vector<int>& channels, vector<std::string>& pars)");

    if (pars.size() < 1)
    {
        MSG_ERROR("Expecting 1 parameters but got none! Ignoring command.");
        return;
    }

    TError::clear();
    int btState = atoi(pars[0].c_str());
    string order;

    if (pars.size() > 1)
    {
        string ord = pars[1];
        // Convert the numbers into the expected draw order
        for (size_t i = 0; i < ord.length(); i++)
        {
            if (ord.at(i) >= '1' && ord.at(i) <= '5')
            {
                char hv0[32];
                snprintf(hv0, sizeof(hv0), "%02d", (int)(ord.at(i) - '0'));
                order.append(hv0);
            }
            else
            {
                MSG_ERROR("Illegal order number " << ord.substr(i, 1) << "!");
                return;
            }
        }

        if (order.length() != 10)
        {
            MSG_ERROR("Expected 5 order numbers but got " << (order.length() / 2)<< "!");
            return;
        }
    }

    vector<MAP_T> map = findButtons(port, channels);

    if (TError::isError() || map.empty())
        return;

    vector<Button::TButton *> buttons = collectButtons(map);

    if (buttons.size() > 0)
    {
        vector<Button::TButton *>::iterator mapIter;

        for (mapIter = buttons.begin(); mapIter != buttons.end(); mapIter++)
        {
            Button::TButton *bt = *mapIter;
            setButtonCallbacks(bt);

            if (btState == 0)       // All instances?
            {
                int bst = bt->getNumberInstances();

                for (int i = 0; i < bst; i++)
                    bt->setDrawOrder(order, i);
            }
            else
                bt->setDrawOrder(order, btState - 1);
        }
    }
}

/**
 * Set the feedback type of the button.
 * ONLY works on General-type buttons.
 */
void TPageManager::doBFB(int port, vector<int>& channels, vector<std::string>& pars)
{
    DECL_TRACER("TPageManager::doBFB(int port, vector<int>& channels, vector<std::string>& pars)");

    if (pars.size() < 1)
    {
        MSG_ERROR("Expecting 1 parameters but got none! Ignoring command.");
        return;
    }

    TError::clear();
    Button::FEEDBACK type = Button::FB_NONE;
    string stype = pars[0];
    vector<string> stypes = { "None", "Channel", "Invert", "On", "Momentary", "Blink" };
    vector<string>::iterator iter;
    int i = 0;

    for (iter = stypes.begin(); iter != stypes.end(); ++iter)
    {
        if (strCaseCompare(stype, *iter) == 0)
        {
            type = (Button::FEEDBACK)i;
            break;
        }

        i++;
    }

    vector<MAP_T> map = findButtons(port, channels);

    if (TError::isError() || map.empty())
        return;

    vector<Button::TButton *> buttons = collectButtons(map);

    if (buttons.size() > 0)
    {
        vector<Button::TButton *>::iterator mapIter;

        for (mapIter = buttons.begin(); mapIter != buttons.end(); mapIter++)
        {
            Button::TButton *bt = *mapIter;
            setButtonCallbacks(bt);
            bt->setFeedback(type);
        }
    }
}

void TPageManager::doBMC(int port, vector<int>& channels, vector<std::string>& pars)
{
    DECL_TRACER("TPageManager::doBMC(int port, vector<int>& channels, vector<std::string>& pars)");

    if (pars.size() < 5)
    {
        MSG_ERROR("Expecting 5 parameters but got " << pars.size() << ". Ignoring command.");
        return;
    }

    TError::clear();
    int btState = atoi(pars[0].c_str());
    int src_port = atoi(pars[1].c_str());
    int src_addr = atoi(pars[2].c_str());
    int src_state = atoi(pars[3].c_str());
    string src_codes = pars[4];
    vector<int> src_channel;
    src_channel.push_back(src_addr);

    vector<MAP_T> src_map = findButtons(src_port, src_channel);

    if (src_map.size() == 0)
    {
        MSG_WARNING("Button <" << TConfig::getChannel() << ":" << src_port << ":" << TConfig::getSystem() << ">:" << src_addr << " does not exist!");
        return;
    }

    vector<Button::TButton *>src_buttons = collectButtons(src_map);

    if (src_buttons.size() == 0)
    {
        MSG_WARNING("Button <" << TConfig::getChannel() << ":" << src_port << ":" << TConfig::getSystem() << ">:" << src_addr << " does not exist!");
        return;
    }

    if (src_buttons[0]->getNumberInstances() < src_state)
    {
        MSG_WARNING("Button <" << TConfig::getChannel() << ":" << src_port << ":" << TConfig::getSystem() << ">:" << src_addr << " has less then " << src_state << " elements.");
        return;
    }

    if (src_state < 1)
    {
        MSG_WARNING("Button <" << TConfig::getChannel() << ":" << src_port << ":" << TConfig::getSystem() << ">:" << src_addr << " has invalid source state " << src_state << ".");
        return;
    }

    src_state--;

    if (btState > 0)
        btState--;

    vector<MAP_T> map = findButtons(port, channels);
    vector<Button::TButton *> buttons = collectButtons(map);
    //                        0     1     2     3     4     5     6     7
    vector<string>codes = { "BM", "BR", "CB", "CF", "CT", "EC", "EF", "FT",
                            "IC", "JB", "JI", "JT", "LN", "OP", "SO", "TX", // 8 - 15
                            "VI", "WW" };   // 16, 17

    for (size_t ibuttons = 0; ibuttons < buttons.size(); ibuttons++)
    {
        vector<string>::iterator iter;
        int idx = 0;

        for (iter = codes.begin(); iter != codes.end(); ++iter)
        {
            if (src_codes.find(*iter) != string::npos)
            {
                int j, x, y;

                switch(idx)
                {
                    case 0: buttons[ibuttons]->setBitmap(src_buttons[0]->getBitmapName(src_state), btState); break;
                    case 1: buttons[ibuttons]->setBorderStyle(src_buttons[0]->getBorderStyle(src_state), btState); break;
                    case 2: buttons[ibuttons]->setBorderColor(src_buttons[0]->getBorderColor(src_state), btState); break;
                    case 3: buttons[ibuttons]->setFillColor(src_buttons[0]->getFillColor(src_state), btState); break;
                    case 4: buttons[ibuttons]->setTextColor(src_buttons[0]->getTextColor(src_state), btState); break;
                    case 5: buttons[ibuttons]->setTextEffectColor(src_buttons[0]->getTextEffectColor(src_state), btState); break;
                    case 6: buttons[ibuttons]->setTextEffect(src_buttons[0]->getTextEffect(src_state), btState); break;
                    case 7: buttons[ibuttons]->setFontIndex(src_buttons[0]->getFontIndex(src_state), btState); break;
                    case 8: buttons[ibuttons]->setIcon(src_buttons[0]->getIconIndex(src_state), btState); break;

                    case 9:
                        j = src_buttons[0]->getBitmapJustification(&x, &y, src_state);
                        buttons[ibuttons]->setBitmapJustification(j, x, y, btState);
                    break;

                    case 10:
                        j = src_buttons[0]->getIconJustification(&x, &y, src_state);
                        buttons[ibuttons]->setIconJustification(j, x, y, btState);
                    break;

                    case 11:
                        j = src_buttons[0]->getTextJustification(&x, &y, src_state);
                        buttons[ibuttons]->setTextJustification(j, x, y, btState);
                    break;

                    case 12: MSG_INFO("\"Lines of video removed\" not supported!"); break;
                    case 13: buttons[ibuttons]->setOpacity(src_buttons[0]->getOpacity(src_state), btState); break;
                    case 14: buttons[ibuttons]->setSound(src_buttons[0]->getSound(src_state), btState); break;
                    case 15: buttons[ibuttons]->setText(src_buttons [0]->getText(src_state), btState); break;
                    case 16: MSG_INFO("\"Video slot ID\" not supported!"); break;
                    case 17: buttons[ibuttons]->setTextWordWrap(src_buttons[0]->getTextWordWrap(src_state), btState); break;
                }
            }

            idx++;
        }
    }
}

/**
 * Set the maximum length of the text area button. If this value is set to
 * zero (0), the text area has no max length. The maximum length available is
 * 2000. This is only for a Text area input button and not for a Text area input
 * masking button.
 */
void TPageManager::doBML(int port, vector<int>& channels, vector<string>& pars)
{
    DECL_TRACER("TPageManager::doBML(int port, vector<int>& channels, vector<string>& pars)");

    if (pars.size() < 1)
    {
        MSG_ERROR("Expecting 1 parameter but got " << pars.size() << "! Ignoring command.");
        return;
    }

    TError::clear();
    int maxLen = atoi(pars[0].c_str());

    if (maxLen < 0 || maxLen > 2000)
    {
        MSG_WARNING("Got illegal length of text area! [" << maxLen << "]");
        return;
    }

    vector<MAP_T> map = findButtons(port, channels);

    if (TError::isError() || map.empty())
        return;

    vector<Button::TButton *> buttons = collectButtons(map);

    if (buttons.size() > 0)
    {
        vector<Button::TButton *>::iterator mapIter;

        for (mapIter = buttons.begin(); mapIter != buttons.end(); mapIter++)
        {
            Button::TButton *bt = *mapIter;
            bt->setTextMaxChars(maxLen);
        }
    }
}

/**
 * Assign a picture to those buttons with a defined address range.
 */
void TPageManager::doBMP(int port, vector<int>& channels, vector<string>& pars)
{
    DECL_TRACER("TPageManager::doBMP(int port, vector<int>& channels, vector<string>& pars)");

    if (pars.size() < 2)
    {
        MSG_ERROR("Expecting 2 parameters but got " << pars.size() << "! Ignoring command.");
        return;
    }

    TError::clear();
    int btState = atoi(pars[0].c_str());
    string bitmap = pars[1];
    // If this is a G5 command, we may have up to 2 additional parameters.
    int slot = -1, justify = -1, jx = 0, jy = 0;

    if (pars.size() > 2)
    {
        slot = atoi(pars[2].c_str());

        if (pars.size() >= 4)
        {
            justify = atoi(pars[4].c_str());

            if (justify == 0)
            {
                if (pars.size() >= 5)
                    jx = atoi(pars[5].c_str());

                if (pars.size() >= 6)
                    jy = atoi(pars[6].c_str());
            }
        }
    }

    vector<MAP_T> map = findButtons(port, channels);

    if (TError::isError() || map.empty())
        return;

    vector<Button::TButton *> buttons = collectButtons(map);

    if (buttons.size() > 0)
    {
        vector<Button::TButton *>::iterator mapIter;

        for (mapIter = buttons.begin(); mapIter != buttons.end(); mapIter++)
        {
            Button::TButton *bt = *mapIter;
            setButtonCallbacks(bt);

            if (btState == 0)       // All instances?
            {
                int bst = bt->getNumberInstances();
                MSG_DEBUG("Setting bitmap " << bitmap << " on all " << bst << " instances...");

                for (int i = 0; i < bst; i++)
                {
                    if (justify >= 0)
                    {
                        if (slot == 2)
                            bt->setIconJustification(justify, jx, jy, i);
                        else
                            bt->setBitmapJustification(justify, jx, jy, i);
                    }

                    if (slot >= 0)
                    {
                        switch(slot)
                        {
                            case 0: bt->setCameleon(bitmap, i); break;
                            case 2: bt->setIcon(bitmap, i); break;  // On G4 we have no bitmap layer. Therefor we use layer 2 as icon layer.
                            default:
                                bt->setBitmap(bitmap, i);
                        }
                    }
                    else
                        bt->setBitmap(bitmap, i);
                }
            }
            else
            {
                if (justify >= 0)
                {
                    if (slot == 2)
                        bt->setIconJustification(justify, jx, jy, btState);
                    else
                        bt->setBitmapJustification(justify, jx, jy, btState);
                }

                if (slot >= 0)
                {
                    switch(slot)
                    {
                        case 0: bt->setCameleon(bitmap, btState); break;
                        case 2: bt->setIcon(bitmap, btState); break;      // On G4 we have no bitmap layer. Therefor we use layer 2 as icon layer.
                        default:
                            bt->setBitmap(bitmap, btState);
                    }
                }
                else
                    bt->setBitmap(bitmap, btState);
            }
        }
    }
}

void TPageManager::getBMP(int port, vector<int> &channels, vector<string> &pars)
{
    DECL_TRACER("TPageManager::getBMP(int port, vector<int> &channels, vector<string> &pars)");

    if (pars.size() < 1)
    {
        MSG_ERROR("Expecting 1 parameters but got none! Ignoring command.");
        return;
    }

    TError::clear();
    int btState = atoi(pars[0].c_str());
    string bmp;

    vector<MAP_T> map = findButtons(port, channels);

    if (TError::isError() || map.empty())
        return;

    vector<Button::TButton *> buttons = collectButtons(map);

    if (buttons.size() > 0)
    {
        Button::TButton *bt = buttons[0];

        if (btState == 0)       // All instances?
        {
            int bst = bt->getNumberInstances();

            for (int i = 0; i < bst; i++)
            {
                bmp = bt->getBitmapName(i);

                if (bmp.empty())
                    continue;

                sendCustomEvent(i + 1, bmp.length(), 0, bmp, 1002, bt->getChannelPort(), bt->getChannelNumber());
            }
        }
        else
        {
            bmp = bt->getTextColor(btState-1);
            sendCustomEvent(btState, bmp.length(), 0, bmp, 1002, bt->getChannelPort(), bt->getChannelNumber());
        }
    }
}

/**
 * Set the button opacity. The button opacity can be specified as a decimal
 * between 0 - 255, where zero (0) is invisible and 255 is opaque, or as a
 * HEX code, as used in the color commands by preceding the HEX code with
 * the # sign. In this case, #00 becomes invisible and #FF becomes opaque.
 * If the opacity is set to zero (0), this does not make the button inactive,
 * only invisible.
 */
void TPageManager::doBOP(int port, vector<int>& channels, vector<string>& pars)
{
    DECL_TRACER("TPageManager::doBOP(int port, vector<int>& channels, vector<string>& pars)");

    if (pars.size() < 2)
    {
        MSG_ERROR("Expecting 2 parameters but got " << pars.size() << "! Ignoring command.");
        return;
    }

    TError::clear();
    int btState = atoi(pars[0].c_str());
    int btOpacity = 0;

    if (pars[1].at(0) == '#')
        btOpacity = (int)strtol(pars[1].substr(1).c_str(), NULL, 16);
    else
        btOpacity = atoi(pars[1].c_str());

    vector<MAP_T> map = findButtons(port, channels);

    if (TError::isError() || map.empty())
        return;

    vector<Button::TButton *> buttons = collectButtons(map);

    if (buttons.size() > 0)
    {
        vector<Button::TButton *>::iterator mapIter;

        for (mapIter = buttons.begin(); mapIter != buttons.end(); mapIter++)
        {
            Button::TButton *bt = *mapIter;
            setButtonCallbacks(bt);

            if (btState == 0)       // All instances?
            {
                int bst = bt->getNumberInstances();
                MSG_DEBUG("Setting opacity " << btOpacity << " on all " << bst << " instances...");

                for (int i = 0; i < bst; i++)
                    bt->setOpacity(btOpacity, i);
            }
            else
                bt->setOpacity(btOpacity, btState);
        }
    }
}

void TPageManager::getBOP(int port, vector<int>& channels, vector<string>& pars)
{
    DECL_TRACER("TPageManager::getBOP(int port, vector<int>& channels, vector<string>& pars)");

    if (pars.size() < 1)
    {
        MSG_ERROR("Expecting 1 parameters but got none! Ignoring command.");
        return;
    }

    TError::clear();
    int btState = atoi(pars[0].c_str());

    vector<MAP_T> map = findButtons(port, channels);

    if (TError::isError() || map.empty())
        return;

    vector<Button::TButton *> buttons = collectButtons(map);

    if (buttons.size() > 0)
    {
        Button::TButton *bt = buttons[0];

        if (btState == 0)       // All instances?
        {
            int bst = bt->getNumberInstances();

            for (int i = 0; i < bst; i++)
            {
                int oo = bt->getOpacity(i);
                sendCustomEvent(i + 1, oo, 0, "", 1015, bt->getChannelPort(), bt->getChannelNumber());
            }
        }
        else
        {
            int oo = bt->getOpacity(btState-1);
            sendCustomEvent(btState, oo, 0, "", 1015, bt->getChannelPort(), bt->getChannelNumber());
        }
    }
}

void TPageManager::doBOR(int port, vector<int>& channels, vector<string>& pars)
{
    DECL_TRACER("TPageManager::doBOR(int port, vector<int>& channels, vector<string>& pars)");

    if (pars.size() < 1)
    {
        MSG_ERROR("Expecting at least 1 parameter but got " << pars.size() << "! Ignoring command.");
        return;
    }

    TError::clear();
    // Numbers of styles from 0 to 41
    vector<string> styles = { "No border", "No border", "Single line", "Double line", "Quad line",
                              "Circle 15", "Circle 25", "Single line", "Double line",
                              "Quad line", "Picture frame", "Picture frame", "Double line",
                              "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A",
                              "Bevel-S", "Bevel-M", "Circle 15", "Circle 25", "Neon inactive-S",
                              "Neon inactive-M", "Neon inactive-L", "Neon inactive-L",
                              "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A", "N/A",
                              "Diamond 55", "Diamond 56" };
    string bor = pars[0];
    string border = "None";
    int ibor = -1;

    if (bor.at(0) >= '0' && bor.at(0) <= '9')
        ibor = atoi(bor.c_str());
    else
    {
        vector<string>::iterator iter;
        int i = 0;

        for (iter = styles.begin(); iter != styles.end(); ++iter)
        {
            if (strCaseCompare(*iter, bor) == 0)
            {
                ibor = i;
                break;
            }

            i++;
        }
    }

    if (ibor < 0 || ibor > 41)
    {
        MSG_ERROR("Invalid border type " << bor << "!");
        return;
    }

    switch (ibor)
    {
        case 20: border = "Bevel Raised -S"; break;
        case 21: border = "Bevel Raised -M"; break;
        case 24: border = "AMX Elite Raised -S"; break;
        case 25: border = "AMX Elite Inset -S"; break;
        case 26: border = "AMX Elite Raised -M"; break;
        case 27: border = "AMX Elite Inset -M"; break;
        case 28: border = "AMX Elite Raised -L"; break;
        case 29: border = "AMX Elite Inset -L"; break;
        case 30: border = "Circle 35"; break;
        case 31: border = "Circle 45"; break;
        case 32: border = "Circle 55"; break;
        case 33: border = "Circle 65"; break;
        case 34: border = "Circle 75"; break;
        case 35: border = "Circle 85"; break;
        case 36: border = "Circle 95"; break;
        case 37: border = "Circle 105"; break;
        case 38: border = "Circle 115"; break;
        case 39: border = "Circle 125"; break;
        default:
            border = styles[ibor];
    }

    vector<MAP_T> map = findButtons(port, channels);

    if (TError::isError() || map.empty())
        return;

    vector<Button::TButton *> buttons = collectButtons(map);

    if (buttons.size() > 0)
    {
        vector<Button::TButton *>::iterator mapIter;

        for (mapIter = buttons.begin(); mapIter != buttons.end(); mapIter++)
        {
            Button::TButton *bt = *mapIter;
            setButtonCallbacks(bt);
            bt->setBorderStyle(border);
        }
    }
}

void TPageManager::doBOS(int port, vector<int>& channels, vector<string>& pars)
{
    DECL_TRACER("TPageManager::doBOS(int port, vector<int>& channels, vector<string>& pars)");

    if (pars.size() < 2)
    {
        MSG_ERROR("Expecting at least 2 parameters but got " << pars.size() << "! Ignoring command.");
        return;
    }

    TError::clear();
    int btState = atoi(pars[0].c_str());
    int videoState = atoi(pars[1].c_str());

    vector<MAP_T> map = findButtons(port, channels);

    if (TError::isError() || map.empty())
        return;

    vector<Button::TButton *> buttons = collectButtons(map);

    if (buttons.size() > 0)
    {
        vector<Button::TButton *>::iterator mapIter;

        for (mapIter = buttons.begin(); mapIter != buttons.end(); mapIter++)
        {
            Button::TButton *bt = *mapIter;

            if (btState == 0)       // All instances?
                bt->setDynamic(videoState);
            else
                bt->setDynamic(videoState, btState-1);
        }
    }
}

/**
 * Set the border of a button state/states.
 * The border names are available through the TPDesign4 border-name drop-down
 * list.
 */
void TPageManager::doBRD(int port, vector<int>& channels, vector<string>& pars)
{
    DECL_TRACER("TPageManager::doBRD(int port, vector<int>& channels, vector<string>& pars)");

    if (pars.size() < 1)
    {
        MSG_ERROR("Expecting at least 1 parameter but got " << pars.size() << "! Ignoring command.");
        return;
    }

    TError::clear();
    int btState = atoi(pars[0].c_str());
    string border = "None";

    if (pars.size() > 1)
        border = pars[1];

    vector<MAP_T> map = findButtons(port, channels);

    if (TError::isError() || map.empty())
        return;

    vector<Button::TButton *> buttons = collectButtons(map);

    if (buttons.size() > 0)
    {
        vector<Button::TButton *>::iterator mapIter;

        for (mapIter = buttons.begin(); mapIter != buttons.end(); mapIter++)
        {
            Button::TButton *bt = *mapIter;
            setButtonCallbacks(bt);

            if (btState == 0)       // All instances?
            {
                int bst = bt->getNumberInstances();

                for (int i = 0; i < bst; i++)
                    bt->setBorderStyle(border, i+1);
            }
            else
                bt->setBorderStyle(border, btState);
        }
    }
}

void TPageManager::getBRD(int port, vector<int>& channels, vector<string>& pars)
{
    DECL_TRACER("TPageManager::getBRD(int port, vector<int>& channels, vector<string>& pars)");

    if (pars.size() < 1)
    {
        MSG_ERROR("Expecting at least 1 parameter but got " << pars.size() << "! Ignoring command.");
        return;
    }

    TError::clear();
    int btState = atoi(pars[0].c_str());

    vector<MAP_T> map = findButtons(port, channels);

    if (TError::isError() || map.empty())
        return;

    vector<Button::TButton *> buttons = collectButtons(map);

    if (buttons.size() > 0)
    {
        Button::TButton *bt = buttons[0];

        if (btState == 0)       // All instances?
        {
            int bst = bt->getNumberInstances();

            for (int i = 0; i < bst; i++)
            {
                string bname = bt->getBorderStyle(i);
                sendCustomEvent(i + 1, bname.length(), 0, bname, 1014, bt->getChannelPort(), bt->getChannelNumber());
            }
        }
        else
        {
            string bname = bt->getBorderStyle(btState-1);
            sendCustomEvent(btState, bname.length(), 0, bname, 1014, bt->getChannelPort(), bt->getChannelNumber());
        }
    }
}

/**
 * Set the button size and its position on the page.
 */
void TPageManager::doBSP(int port, vector<int>& channels, vector<string>& pars)
{
    DECL_TRACER("TPageManager::doBSP(int port, vector<int>& channels, vector<string>& pars)");

    if (pars.size() < 1)
    {
        MSG_ERROR("Expecting at least 1 parameter but got " << pars.size() << "! Ignoring command.");
        return;
    }

    TError::clear();
    bool bLeft = false, bTop = false, bRight = false, bBottom = false;
    int x, y;

    if (pars.size() > 0)
    {
        vector<string>::iterator iter;

        for (iter = pars.begin(); iter != pars.end(); iter++)
        {
            if (iter->compare("left") == 0)
                bLeft = true;
            else if (iter->compare("top") == 0)
                bTop = true;
            else if (iter->compare("right") == 0)
                bRight = true;
            else if (iter->compare("bottom") == 0)
                bBottom = true;
        }
    }

    vector<MAP_T> map = findButtons(port, channels);

    if (TError::isError() || map.empty())
        return;

    vector<Button::TButton *> buttons = collectButtons(map);

    if (buttons.size() > 0)
    {
        vector<Button::TButton *>::iterator mapIter;

        for (mapIter = buttons.begin(); mapIter != buttons.end(); mapIter++)
        {
            Button::TButton *bt = *mapIter;
            setButtonCallbacks(bt);

            if (bLeft)
                x = 0;

            if (bTop)
                y = 0;

            if (bRight)
            {
                ulong handle = bt->getHandle();
                int parentID = (handle >> 16) & 0x0000ffff;
                int pwidth = 0;

                if (parentID < 500)
                {
                    TPage *pg = getPage(parentID);

                    if (!pg)
                    {
                        MSG_ERROR("Internal error: Page " << parentID << " not found!");
                        return;
                    }

                    pwidth = pg->getWidth();
                }
                else
                {
                    TSubPage *spg = getSubPage(parentID);

                    if (!spg)
                    {
                        MSG_ERROR("Internal error: Subpage " << parentID << " not found!");
                        return;
                    }

                    pwidth = spg->getWidth();
                }

                x = pwidth - bt->getWidth();
            }

            if (bBottom)
            {
                ulong handle = bt->getHandle();
                int parentID = (handle >> 16) & 0x0000ffff;
                int pheight = 0;

                if (parentID < 500)
                {
                    TPage *pg = getPage(parentID);

                    if (!pg)
                    {
                        MSG_ERROR("Internal error: Page " << parentID << " not found!");
                        return;
                    }

                    pheight = pg->getHeight();
                }
                else
                {
                    TSubPage *spg = getSubPage(parentID);

                    if (!spg)
                    {
                        MSG_ERROR("Internal error: Subpage " << parentID << " not found!");
                        return;
                    }

                    pheight = spg->getHeight();
                }

                y = pheight - bt->getHeight();
            }

            bt->setLeftTop(x, y);
        }
    }
}

/**
 * Submit text for text area buttons. This command causes the text areas to
 * send their text as strings to the NetLinx Master.
 */
void TPageManager::doBSM(int port, vector<int>& channels, vector<string>&)
{
    DECL_TRACER("TPageManager::doBSM(int port, vector<int>& channels, vector<string>& pars)");

    TError::clear();
    vector<MAP_T> map = findButtons(port, channels);

    if (TError::isError() || map.empty())
        return;

    vector<Button::TButton *> buttons = collectButtons(map);

    if (buttons.size() > 0)
    {
        vector<Button::TButton *>::iterator mapIter;

        for (mapIter = buttons.begin(); mapIter != buttons.end(); mapIter++)
        {
            Button::TButton *bt = *mapIter;

            if (bt->getButtonType() != Button::TEXT_INPUT && bt->getButtonType() != Button::GENERAL)
                return;

            amx::ANET_SEND scmd;
            scmd.port = bt->getChannelPort();
            scmd.channel = bt->getChannelNumber();
            scmd.ID = scmd.channel;
            scmd.msg = bt->getText(0);
            scmd.MC = 0x008b;       // string value

            if (gAmxNet)
                gAmxNet->sendCommand(scmd);
            else
                MSG_WARNING("Missing global class TAmxNet. Can't send message!");

        }
    }
}

/**
 * Set the sound played when a button is pressed. If the sound name is blank
 * the sound is then cleared. If the sound name is not matched, the button
 * sound is not changed.
 */
void TPageManager::doBSO(int port, vector<int>& channels, vector<string>& pars)
{
    DECL_TRACER("TPageManager::doBSO(int port, vector<int>& channels, vector<string>& pars)");

    if (pars.size() < 2)
    {
        MSG_ERROR("Expecting 2 parameters but got " << pars.size() << "! Ignoring command.");
        return;
    }

    if (!gPrjResources)
        return;

    TError::clear();
    int btState = atoi(pars[0].c_str());
    string sound = pars[1];

    if (!soundExist(sound))
        return;

    vector<MAP_T> map = findButtons(port, channels);

    if (TError::isError() || map.empty())
        return;

    vector<Button::TButton *> buttons = collectButtons(map);

    if (buttons.size() > 0)
    {
        vector<Button::TButton *>::iterator mapIter;

        for (mapIter = buttons.begin(); mapIter != buttons.end(); mapIter++)
        {
            Button::TButton *bt = *mapIter;

            if (btState == 0)
            {
                int bst = bt->getNumberInstances();

                for (int i = 0; i < bst; i++)
                    bt->setSound(sound, i);
            }
            else
                bt->setSound(sound, btState-1);
        }
    }
}

/**
 * Set the button word wrap feature to those buttons with a defined address
 * range. By default, word-wrap is Off.
 */
void TPageManager::doBWW(int port, vector<int>& channels, vector<string>& pars)
{
    DECL_TRACER("TPageManager::doBWW(int port, vector<int>& channels, vector<string>& pars)");

    if (pars.size() < 1)
    {
        MSG_ERROR("Expecting 1 parameter but got " << pars.size() << "! Ignoring command.");
        return;
    }

    TError::clear();
    int btState = atoi(pars[0].c_str());

    vector<MAP_T> map = findButtons(port, channels);

    if (TError::isError() || map.empty())
        return;

    vector<Button::TButton *> buttons = collectButtons(map);

    if (buttons.size() > 0)
    {
        vector<Button::TButton *>::iterator mapIter;

        for (mapIter = buttons.begin(); mapIter != buttons.end(); mapIter++)
        {
            Button::TButton *bt = *mapIter;
            setButtonCallbacks(bt);

            if (btState == 0)       // All instances?
            {
                int bst = bt->getNumberInstances();
                MSG_DEBUG("Setting word wrap on all " << bst << " instances...");

                for (int i = 0; i < bst; i++)
                    bt->setTextWordWrap(true, i);
            }
            else
                bt->setTextWordWrap(true, btState - 1);
        }
    }
}

void TPageManager::getBWW(int port, vector<int>& channels, vector<string>& pars)
{
    DECL_TRACER("TPageManager::getBWW(int port, vector<int>& channels, vector<string>& pars)");

    if (pars.size() < 1)
    {
        MSG_ERROR("Expecting at least 1 parameter but got " << pars.size() << "! Ignoring command.");
        return;
    }

    TError::clear();
    int btState = atoi(pars[0].c_str());

    vector<MAP_T> map = findButtons(port, channels);

    if (TError::isError() || map.empty())
        return;

    vector<Button::TButton *> buttons = collectButtons(map);

    if (buttons.size() > 0)
    {
        Button::TButton *bt = buttons[0];

        if (btState == 0)       // All instances?
        {
            int bst = bt->getNumberInstances();

            for (int i = 0; i < bst; i++)
                sendCustomEvent(i + 1, bt->getTextWordWrap(i), 0, "", 1010, bt->getChannelPort(), bt->getChannelNumber());
        }
        else
            sendCustomEvent(btState, bt->getTextWordWrap(btState-1), 0, "", 1010, bt->getChannelPort(), bt->getChannelNumber());
    }
}

/**
 * Clear all page flips from a button.
 */
void TPageManager::doCPF(int port, vector<int>& channels, vector<string>&)
{
    DECL_TRACER("TPageManager::doCPF(int port, vector<int>& channels, vector<string>& pars)");

    TError::clear();
    vector<MAP_T> map = findButtons(port, channels);

    if (TError::isError() || map.empty())
        return;

    vector<Button::TButton *> buttons = collectButtons(map);

    if (buttons.size() > 0)
    {
        vector<Button::TButton *>::iterator mapIter;

        for (mapIter = buttons.begin(); mapIter != buttons.end(); mapIter++)
        {
            Button::TButton *bt = *mapIter;
            setButtonCallbacks(bt);
            bt->clearPushFunctions();
        }
    }
}

/**
 * Delete page flips from button if it already exists.
 */
void TPageManager::doDPF(int port, vector<int>& channels, vector<string>& pars)
{
    DECL_TRACER("TPageManager::doDPF(int port, vector<int>& channels, vector<string>& pars)");

    if (pars.size() < 1)
    {
        MSG_ERROR("Expecting at least 1 parameter but got " << pars.size() << "! Ignoring command.");
        return;
    }

    TError::clear();
    string action = pars[0];
    string pname;

    if (pars.size() >= 2)
    {
        pname = pars[1];
        vector<Button::TButton *> list;
        // First we search for a subpage because this is more likely
        TSubPage *spg = getSubPage(pname);

        if (spg)
            list = spg->getButtons(port, channels[0]);
        else    // Then for a page
        {
            TPage *pg = getPage(pname);

            if (pg)
                list = pg->getButtons(port, channels[0]);
            else
            {
                MSG_WARNING("The name " << pname << " doesn't name either a page or a subpage!");
                return;
            }
        }

        if (list.empty())
            return;

        vector<Button::TButton *>::iterator it;

        for (it = list.begin(); it != list.end(); it++)
        {
            Button::TButton *bt = *it;
            setButtonCallbacks(bt);
            bt->clearPushFunction(action);
        }

        return;
    }

    // Here we don't have a page name
    vector<MAP_T> map = findButtons(port, channels);

    if (TError::isError() || map.empty())
        return;

    vector<Button::TButton *> buttons = collectButtons(map);

    if (buttons.size() > 0)
    {
        vector<Button::TButton *>::iterator mapIter;

        for (mapIter = buttons.begin(); mapIter != buttons.end(); mapIter++)
        {
            Button::TButton *bt = *mapIter;
            setButtonCallbacks(bt);
            bt->clearPushFunction(action);
        }
    }
}

/**
 * Enable or disable buttons with a set variable text range.
 */
void TPageManager::doENA(int port, vector<int>& channels, vector<string>& pars)
{
    DECL_TRACER("TPageManager::doENA(int port, vector<int>& channels, vector<string>& pars)");

    if (pars.empty())
    {
        MSG_ERROR("Expecting 1 parameter but got none! Ignoring command.");
        return;
    }

    TError::clear();
    int cvalue = atoi(pars[0].c_str());

    vector<MAP_T> map = findButtons(port, channels);

    if (TError::isError() || map.empty())
        return;

    vector<Button::TButton *> buttons = collectButtons(map);

    if (buttons.size() > 0)
    {
        vector<Button::TButton *>::iterator mapIter;

        for (mapIter = buttons.begin(); mapIter != buttons.end(); mapIter++)
        {
            Button::TButton *bt = *mapIter;
            setButtonCallbacks(bt);
            bt->setEnable(((cvalue)?true:false));
        }
    }
}

/**
 * Set a font to a specific Font ID value for those buttons with a defined
 * address range. Font ID numbers are generated by the TPDesign4 programmers
 * report.
 */
void TPageManager::doFON(int port, vector<int>& channels, vector<string>& pars)
{
    DECL_TRACER("TPageManager::doFON(int port, vector<int>& channels, vector<string>& pars)");

    if (pars.size() < 2)
    {
        MSG_ERROR("Expecting 2 parameters but got " << pars.size() << "! Ignoring command.");
        return;
    }

    TError::clear();
    int btState = atoi(pars[0].c_str());
    int fvalue = atoi(pars[1].c_str());

    vector<MAP_T> map = findButtons(port, channels);

    if (TError::isError() || map.empty())
        return;

    vector<Button::TButton *> buttons = collectButtons(map);

    if (buttons.size() > 0)
    {
        vector<Button::TButton *>::iterator mapIter;

        for (mapIter = buttons.begin(); mapIter != buttons.end(); mapIter++)
        {
            Button::TButton *bt = *mapIter;
            setButtonCallbacks(bt);

            if (btState == 0)       // All instances?
            {
                int bst = bt->getNumberInstances();
                MSG_DEBUG("Setting font " << fvalue << " on all " << bst << " instances...");

                for (int i = 0; i < bst; i++)
                    bt->setFont(fvalue, i);
            }
            else
                bt->setFont(fvalue, btState - 1);
        }
    }
}

void TPageManager::getFON(int port, vector<int>& channels, vector<string>& pars)
{
    DECL_TRACER("TPageManager::getFON(int port, vector<int>& channels, vector<string>& pars)");

    if (pars.size() < 1)
    {
        MSG_ERROR("Expecting at least 1 parameter but got " << pars.size() << "! Ignoring command.");
        return;
    }

    TError::clear();
    int btState = atoi(pars[0].c_str());

    vector<MAP_T> map = findButtons(port, channels);

    if (TError::isError() || map.empty())
        return;

    vector<Button::TButton *> buttons = collectButtons(map);

    if (buttons.size() > 0)
    {
        Button::TButton *bt = buttons[0];

        if (btState == 0)       // All instances?
        {
            int bst = bt->getNumberInstances();

            for (int i = 0; i < bst; i++)
                sendCustomEvent(i + 1, bt->getFontIndex(i), 0, "", 1007, bt->getChannelPort(), bt->getChannelNumber());
        }
        else
            sendCustomEvent(btState, bt->getFontIndex(btState - 1), 0, "", 1007, bt->getChannelPort(), bt->getChannelNumber());
    }
}

/**
 * Change the bargraph upper limit.
 */
void TPageManager::doGLH(int port, vector<int>& channels, vector<std::string>& pars)
{
    DECL_TRACER("TPageManager::doGLH(int port, vector<int>& channels, vector<std::string>& pars)");

    if (pars.size() < 1)
    {
        MSG_ERROR("Expecting 1 parameter but got " << pars.size() << "! Ignoring command.");
        return;
    }

    TError::clear();
    int limit = atoi(pars[0].c_str());

    if (limit < 1)
    {
        MSG_ERROR("Invalid upper limit " << limit << "!");
        return;
    }

    vector<MAP_T> map = findButtons(port, channels);

    if (TError::isError() || map.empty())
        return;

    vector<Button::TButton *> buttons = collectButtons(map);

    if (buttons.size() > 0)
    {
        vector<Button::TButton *>::iterator mapIter;

        for (mapIter = buttons.begin(); mapIter != buttons.end(); mapIter++)
        {
            Button::TButton *bt = *mapIter;
            setButtonCallbacks(bt);
            bt->setBargraphUpperLimit(limit);
        }
    }
}

/**
 * Change the bargraph lower limit.
 */
void TPageManager::doGLL(int port, vector<int>& channels, vector<std::string>& pars)
{
    DECL_TRACER("TPageManager::doGLL(int port, vector<int>& channels, vector<std::string>& pars)");

    if (pars.size() < 1)
    {
        MSG_ERROR("Expecting 1 parameter but got " << pars.size() << "! Ignoring command.");
        return;
    }

    TError::clear();
    int limit = atoi(pars[0].c_str());

    if (limit < 1)
    {
        MSG_ERROR("Invalid lower limit " << limit << "!");
        return;
    }

    vector<MAP_T> map = findButtons(port, channels);

    if (TError::isError() || map.empty())
        return;

    vector<Button::TButton *> buttons = collectButtons(map);

    if (buttons.size() > 0)
    {
        vector<Button::TButton *>::iterator mapIter;

        for (mapIter = buttons.begin(); mapIter != buttons.end(); mapIter++)
        {
            Button::TButton *bt = *mapIter;
            setButtonCallbacks(bt);
            bt->setBargraphLowerLimit(limit);
        }
    }
}

void TPageManager::doGSC(int port, vector<int>& channels, vector<string>& pars)
{
    DECL_TRACER("TPageManager::doGSC(int port, vector<int>& channels, vector<string>& pars)");

    if (pars.size() < 1)
    {
        MSG_ERROR("Expecting 1 parameter but got " << pars.size() << "! Ignoring command.");
        return;
    }

    TError::clear();
    string color = pars[0];
    vector<MAP_T> map = findButtons(port, channels);

    if (TError::isError() || map.empty())
        return;

    vector<Button::TButton *> buttons = collectButtons(map);

    if (buttons.size() > 0)
    {
        vector<Button::TButton *>::iterator mapIter;

        for (mapIter = buttons.begin(); mapIter != buttons.end(); mapIter++)
        {
            Button::TButton *bt = *mapIter;
            setButtonCallbacks(bt);
            bt->setBargraphSliderColor(color);
        }
    }
}

/**
 * Set the icon to a button.
 */
void TPageManager::doICO(int port, vector<int>& channels, vector<string>& pars)
{
    DECL_TRACER("TPageManager::doICO(int port, vector<int>& channels, vector<string>& pars)");

    if (pars.size() < 2)
    {
        MSG_ERROR("Expecting 2 parameters but got " << pars.size() << "! Ignoring command.");
        return;
    }

    TError::clear();
    int btState = atoi(pars[0].c_str());
    int iconIdx = atoi(pars[1].c_str());

    vector<MAP_T> map = findButtons(port, channels);

    if (TError::isError() || map.empty())
        return;

    vector<Button::TButton *> buttons = collectButtons(map);

    if (buttons.size() > 0)
    {
        vector<Button::TButton *>::iterator mapIter;

        for (mapIter = buttons.begin(); mapIter != buttons.end(); mapIter++)
        {
            Button::TButton *bt = *mapIter;
            setButtonCallbacks(bt);

            if (btState == 0)       // All instances?
            {
                int bst = bt->getNumberInstances();
                MSG_DEBUG("Setting Icon on all " << bst << " instances...");

                for (int i = 0; i < bst; i++)
                {
                    if (iconIdx > 0)
                        bt->setIcon(iconIdx, i);
                    else
                        bt->revokeIcon(i);
                }
            }
            else if (iconIdx > 0)
                bt->setIcon(iconIdx, btState - 1);
            else
                bt->revokeIcon(btState - 1);
        }
    }
}

void TPageManager::getICO(int port, vector<int>& channels, vector<string>& pars)
{
    DECL_TRACER("TPageManager::getICO(int port, vector<int>& channels, vector<string>& pars)");

    if (pars.size() < 1)
    {
        MSG_ERROR("Expecting at least 1 parameter but got " << pars.size() << "! Ignoring command.");
        return;
    }

    TError::clear();
    int btState = atoi(pars[0].c_str());

    vector<MAP_T> map = findButtons(port, channels);

    if (TError::isError() || map.empty())
        return;

    vector<Button::TButton *> buttons = collectButtons(map);

    if (buttons.size() > 0)
    {
        Button::TButton *bt = buttons[0];

        if (btState == 0)       // All instances?
        {
            int bst = bt->getNumberInstances();

            for (int i = 0; i < bst; i++)
                sendCustomEvent(i + 1, bt->getIconIndex(i), 0, "", 1003, bt->getChannelPort(), bt->getChannelNumber());
        }
        else
            sendCustomEvent(btState, bt->getIconIndex(btState - 1), 0, "", 1003, bt->getChannelPort(), bt->getChannelNumber());
    }
}

/**
 * Set bitmap/picture alignment using a numeric keypad layout for those buttons
 * with a defined address range. The alignment of 0 is followed by
 * ',<left>,<top>'. The left and top coordinates are relative to the upper left
 * corner of the button.
 */
void TPageManager::doJSB(int port, vector<int>& channels, vector<string>& pars)
{
    DECL_TRACER("TPageManager::doJSB(int port, vector<int>& channels, vector<string>& pars)");

    if (pars.size() < 2)
    {
        MSG_ERROR("Expecting at least 2 parameters but got less! Ignoring command.");
        return;
    }

    TError::clear();
    int btState = atoi(pars[0].c_str());
    int align = atoi(pars[1].c_str());
    int x = 0, y = 0;

    if (!align && pars.size() >= 3)
    {
        x = atoi(pars[2].c_str());

        if (pars.size() >= 4)
            y = atoi(pars[3].c_str());
    }

    vector<MAP_T> map = findButtons(port, channels);

    if (TError::isError() || map.empty())
        return;

    vector<Button::TButton *> buttons = collectButtons(map);

    if (buttons.size() > 0)
    {
        vector<Button::TButton *>::iterator mapIter;

        for (mapIter = buttons.begin(); mapIter != buttons.end(); mapIter++)
        {
            Button::TButton *bt = *mapIter;

            if (btState == 0)
                bt->setBitmapJustification(align, x, y, -1);
            else
                bt->setBitmapJustification(align, x, y, btState-1);
        }
    }
}

void TPageManager::getJSB(int port, vector<int>& channels, vector<string>& pars)
{
    DECL_TRACER("TPageManager::getJSB(int port, vector<int>& channels, vector<string>& pars)");

    if (pars.size() < 1)
    {
        MSG_ERROR("Expecting at least 1 parameter but got " << pars.size() << "! Ignoring command.");
        return;
    }

    TError::clear();
    int btState = atoi(pars[0].c_str());
    int j, x, y;

    vector<MAP_T> map = findButtons(port, channels);

    if (TError::isError() || map.empty())
        return;

    vector<Button::TButton *> buttons = collectButtons(map);

    if (buttons.size() > 0)
    {
        Button::TButton *bt = buttons[0];

        if (btState == 0)       // All instances?
        {
            int bst = bt->getNumberInstances();

            for (int i = 0; i < bst; i++)
            {
                j = bt->getBitmapJustification(&x, &y, i);
                sendCustomEvent(i + 1, j, 0, "", 1005, bt->getChannelPort(), bt->getChannelNumber());
            }
        }
        else
        {
            j = bt->getBitmapJustification(&x, &y, btState-1);
            sendCustomEvent(btState, j, 0, "", 1005, bt->getChannelPort(), bt->getChannelNumber());
        }
    }
}

/**
 * Set icon alignment using a numeric keypad layout for those buttons with a
 * defined address range. The alignment of 0 is followed by ',<left>,<top>'.
 * The left and top coordinates are relative to the upper left corner of the
 * button.
 */
void TPageManager::doJSI(int port, vector<int>& channels, vector<string>& pars)
{
    DECL_TRACER("TPageManager::doJSB(int port, vector<int>& channels, vector<string>& pars)");

    if (pars.size() < 2)
    {
        MSG_ERROR("Expecting at least 2 parameters but got less! Ignoring command.");
        return;
    }

    TError::clear();
    int btState = atoi(pars[0].c_str());
    int align = atoi(pars[1].c_str());
    int x = 0, y = 0;

    if (!align && pars.size() >= 3)
    {
        x = atoi(pars[2].c_str());

        if (pars.size() >= 4)
            y = atoi(pars[3].c_str());
    }

    vector<MAP_T> map = findButtons(port, channels);

    if (TError::isError() || map.empty())
        return;

    vector<Button::TButton *> buttons = collectButtons(map);

    if (buttons.size() > 0)
    {
        vector<Button::TButton *>::iterator mapIter;

        for (mapIter = buttons.begin(); mapIter != buttons.end(); mapIter++)
        {
            Button::TButton *bt = *mapIter;

            if (btState == 0)
                bt->setIconJustification(align, x, y, -1);
            else
                bt->setIconJustification(align, x, y, btState-1);
        }
    }
}

void TPageManager::getJSI(int port, vector<int>& channels, vector<string>& pars)
{
    DECL_TRACER("TPageManager::getJSB(int port, vector<int>& channels, vector<string>& pars)");

    if (pars.size() < 1)
    {
        MSG_ERROR("Expecting at least 1 parameter but got " << pars.size() << "! Ignoring command.");
        return;
    }

    TError::clear();
    int btState = atoi(pars[0].c_str());
    int j, x, y;

    vector<MAP_T> map = findButtons(port, channels);

    if (TError::isError() || map.empty())
        return;

    vector<Button::TButton *> buttons = collectButtons(map);

    if (buttons.size() > 0)
    {
        Button::TButton *bt = buttons[0];

        if (btState == 0)       // All instances?
        {
            int bst = bt->getNumberInstances();

            for (int i = 0; i < bst; i++)
            {
                j = bt->getIconJustification(&x, &y, i);
                sendCustomEvent(i + 1, j, 0, "", 1006, bt->getChannelPort(), bt->getChannelNumber());
            }
        }
        else
        {
            j = bt->getIconJustification(&x, &y, btState-1);
            sendCustomEvent(btState, j, 0, "", 1006, bt->getChannelPort(), bt->getChannelNumber());
        }
    }
}

void TPageManager::doJST(int port, vector<int>& channels, vector<string>& pars)
{
    DECL_TRACER("TPageManager::doJSB(int port, vector<int>& channels, vector<string>& pars)");

    if (pars.size() < 2)
    {
        MSG_ERROR("Expecting at least 2 parameters but got less! Ignoring command.");
        return;
    }

    TError::clear();
    int btState = atoi(pars[0].c_str());
    int align = atoi(pars[1].c_str());
    int x = 0, y = 0;

    if (!align && pars.size() >= 3)
    {
        x = atoi(pars[2].c_str());

        if (pars.size() >= 4)
            y = atoi(pars[3].c_str());
    }

    vector<MAP_T> map = findButtons(port, channels);

    if (TError::isError() || map.empty())
        return;

    vector<Button::TButton *> buttons = collectButtons(map);

    if (buttons.size() > 0)
    {
        vector<Button::TButton *>::iterator mapIter;

        for (mapIter = buttons.begin(); mapIter != buttons.end(); mapIter++)
        {
            Button::TButton *bt = *mapIter;

            if (btState == 0)
                bt->setTextJustification(align, x, y, -1);
            else
                bt->setTextJustification(align, x, y, btState-1);
        }
    }
}

void TPageManager::getJST(int port, vector<int>& channels, vector<string>& pars)
{
    DECL_TRACER("TPageManager::getJSB(int port, vector<int>& channels, vector<string>& pars)");

    if (pars.size() < 1)
    {
        MSG_ERROR("Expecting at least 1 parameter but got " << pars.size() << "! Ignoring command.");
        return;
    }

    TError::clear();
    int btState = atoi(pars[0].c_str());
    int j, x, y;

    vector<MAP_T> map = findButtons(port, channels);

    if (TError::isError() || map.empty())
        return;

    vector<Button::TButton *> buttons = collectButtons(map);

    if (buttons.size() > 0)
    {
        Button::TButton *bt = buttons[0];

        if (btState == 0)       // All instances?
        {
            int bst = bt->getNumberInstances();

            for (int i = 0; i < bst; i++)
            {
                j = bt->getTextJustification(&x, &y, i);
                sendCustomEvent(i + 1, j, 0, "", 1004, bt->getChannelPort(), bt->getChannelNumber());
            }
        }
        else
        {
            j = bt->getTextJustification(&x, &y, btState-1);
            sendCustomEvent(btState, j, 0, "", 1004, bt->getChannelPort(), bt->getChannelNumber());
        }
    }
}

/**
 * Show or hide a button with a set variable text range.
 */
void TPageManager::doSHO(int port, vector<int>& channels, vector<string>& pars)
{
    DECL_TRACER("TPageManager::doSHO(int port, vector<int>& channels, vector<string>& pars)");

    if (pars.empty())
    {
        MSG_ERROR("Expecting 1 parameter but got none! Ignoring command.");
        return;
    }

    TError::clear();
    int cvalue = atoi(pars[0].c_str());

    vector<MAP_T> map = findButtons(port, channels);

    if (TError::isError() || map.empty())
        return;

    vector<Button::TButton *> buttons = collectButtons(map);

    if (buttons.size() > 0)
    {
        vector<Button::TButton *>::iterator mapIter;

        for (mapIter = buttons.begin(); mapIter != buttons.end(); mapIter++)
        {
            Button::TButton *bt = *mapIter;
            int pgID = (bt->getParent() >> 16) & 0x0000ffff;
            bool pVisible = false;

            if (pgID < 500)
            {
                TPage *pg = getPage(pgID);

                if (pg && pg->isVisilble())
                    pVisible = true;
            }
            else
            {
                TSubPage *pg = getSubPage(pgID);

                if (pg && pg->isVisible())
                    pVisible = true;
            }

            bt->setVisible((cvalue ? true : false));

            if (pVisible)
            {
                setButtonCallbacks(bt);

                if (_setVisible)
                    _setVisible(bt->getHandle(), (cvalue ? true : false));
                else
                    bt->refresh();
            }
        }
    }
}

void TPageManager::doTEC(int port, vector<int>& channels, vector<string>& pars)
{
    DECL_TRACER("TPageManager::doTEC(int port, vector<int>& channels, vector<string>& pars)");

    if (pars.size() < 2)
    {
        MSG_ERROR("Expecting at least 2 parameters but got less! Ignoring command.");
        return;
    }

    TError::clear();
    int btState = atoi(pars[0].c_str());
    string color = pars[1];

    vector<MAP_T> map = findButtons(port, channels);

    if (TError::isError() || map.empty())
        return;

    vector<Button::TButton *> buttons = collectButtons(map);

    if (buttons.size() > 0)
    {
        vector<Button::TButton *>::iterator mapIter;

        for (mapIter = buttons.begin(); mapIter != buttons.end(); mapIter++)
        {
            Button::TButton *bt = *mapIter;

            if (btState == 0)
                bt->setTextEffectColor(color);
            else
                bt->setTextEffectColor(color, btState-1);
        }
    }
}

void TPageManager::getTEC(int port, vector<int>& channels, vector<string>& pars)
{
    DECL_TRACER("TPageManager::getTEC(int port, vector<int>& channels, vector<string>& pars)");

    if (pars.size() < 1)
    {
        MSG_ERROR("Expecting at least 1 parameter but got " << pars.size() << "! Ignoring command.");
        return;
    }

    TError::clear();
    int btState = atoi(pars[0].c_str());

    vector<MAP_T> map = findButtons(port, channels);

    if (TError::isError() || map.empty())
        return;

    vector<Button::TButton *> buttons = collectButtons(map);

    if (buttons.size() > 0)
    {
        Button::TButton *bt = buttons[0];

        if (btState == 0)       // All instances?
        {
            int bst = bt->getNumberInstances();

            for (int i = 0; i < bst; i++)
            {
                string c = bt->getTextEffectColor(i);
                sendCustomEvent(i + 1, c.length(), 0, c, 1009, bt->getChannelPort(), bt->getChannelNumber());
            }
        }
        else
        {
            string c = bt->getTextEffectColor(btState-1);
            sendCustomEvent(btState, c.length(), 0, c, 1009, bt->getChannelPort(), bt->getChannelNumber());
        }
    }
}

void TPageManager::doTEF(int port, vector<int>& channels, vector<string>& pars)
{
    DECL_TRACER("TPageManager::doTEF(int port, vector<int>& channels, vector<string>& pars)");

    if (pars.size() < 2)
    {
        MSG_ERROR("Expecting at least 2 parameters but got less! Ignoring command.");
        return;
    }

    TError::clear();
    int btState = atoi(pars[0].c_str());
    string tef = pars[1];

    vector<MAP_T> map = findButtons(port, channels);

    if (TError::isError() || map.empty())
        return;

    vector<Button::TButton *> buttons = collectButtons(map);

    if (buttons.size() > 0)
    {
        vector<Button::TButton *>::iterator mapIter;

        for (mapIter = buttons.begin(); mapIter != buttons.end(); mapIter++)
        {
            Button::TButton *bt = *mapIter;

            if (btState == 0)
                bt->setTextEffectName(tef);
            else
                bt->setTextEffectName(tef, btState-1);
        }
    }
}

void TPageManager::getTEF(int port, vector<int>& channels, vector<string>& pars)
{
    DECL_TRACER("TPageManager::getTEF(int port, vector<int>& channels, vector<string>& pars)");

    if (pars.size() < 1)
    {
        MSG_ERROR("Expecting at least 1 parameter but got " << pars.size() << "! Ignoring command.");
        return;
    }

    TError::clear();
    int btState = atoi(pars[0].c_str());

    vector<MAP_T> map = findButtons(port, channels);

    if (TError::isError() || map.empty())
        return;

    vector<Button::TButton *> buttons = collectButtons(map);

    if (buttons.size() > 0)
    {
        Button::TButton *bt = buttons[0];

        if (btState == 0)       // All instances?
        {
            int bst = bt->getNumberInstances();

            for (int i = 0; i < bst; i++)
            {
                string c = bt->getTextEffectName(i);
                sendCustomEvent(i + 1, c.length(), 0, c, 1008, bt->getChannelPort(), bt->getChannelNumber());
            }
        }
        else
        {
            string c = bt->getTextEffectName(btState-1);
            sendCustomEvent(btState, c.length(), 0, c, 1008, bt->getChannelPort(), bt->getChannelNumber());
        }
    }
}

/**
 * Assign a text string to those buttons with a defined address range.
 * Sets Non-Unicode text.
 */
void TPageManager::doTXT(int port, vector<int>& channels, vector<string>& pars)
{
    DECL_TRACER("TPageManager::doTXT(int port, vector<int>& channels, vector<string>& pars)");

    if (pars.size() < 1)
    {
        MSG_ERROR("Expecting 1 parameters but got none! Ignoring command.");
        return;
    }

    TError::clear();
    int btState = atoi(pars[0].c_str());
    string text;

    if (pars.size() > 1)
        text = pars[1];

    vector<MAP_T> map = findButtons(port, channels);

    if (TError::isError() || map.empty())
        return;

    vector<Button::TButton *> buttons = collectButtons(map);

    if (buttons.size() > 0)
    {
        vector<Button::TButton *>::iterator mapIter;

        for (mapIter = buttons.begin(); mapIter != buttons.end(); mapIter++)
        {
            Button::TButton *bt = *mapIter;
            setButtonCallbacks(bt);

            if (btState == 0)       // All instances?
            {
                int bst = bt->getNumberInstances();
                MSG_DEBUG("Setting TXT on all " << bst << " instances...");

                for (int i = 0; i < bst; i++)
                    bt->setText(text, i);
            }
            else
                bt->setText(text, btState - 1);
        }
    }
}

void TPageManager::getTXT(int port, vector<int>& channels, vector<string>& pars)
{
    DECL_TRACER("TPageManager::getTXT(int port, vector<int>& channels, vector<string>& pars)");

    if (pars.size() < 1)
    {
        MSG_ERROR("Expecting at least 1 parameter but got " << pars.size() << "! Ignoring command.");
        return;
    }

    TError::clear();
    int btState = atoi(pars[0].c_str());

    vector<MAP_T> map = findButtons(port, channels);

    if (TError::isError() || map.empty())
        return;

    vector<Button::TButton *> buttons = collectButtons(map);

    if (buttons.size() > 0)
    {
        Button::TButton *bt = buttons[0];

        if (btState == 0)       // All instances?
        {
            int bst = bt->getNumberInstances();

            for (int i = 0; i < bst; i++)
            {
                string c = bt->getText(i);
                sendCustomEvent(i + 1, c.length(), 0, c, 1001, bt->getChannelPort(), bt->getChannelNumber());
            }
        }
        else
        {
            string c = bt->getText(btState-1);
            sendCustomEvent(btState, c.length(), 0, c, 1001, bt->getChannelPort(), bt->getChannelNumber());
        }
    }
}

/*
 * Set button state legacy unicode text command.
 *
 * Set Unicode text in the legacy G4 format. For the ^UNI command, the Unicode
 * text is sent as ASCII-HEX nibbles.
 */
void TPageManager::doUNI(int port, vector<int>& channels, vector<string>& pars)
{
    DECL_TRACER("TPageManager::doUNI(int port, vector<int>& channels, vector<string>& pars)");

    if (pars.size() < 1)
    {
        MSG_ERROR("Expecting 1 parameters but got none! Ignoring command.");
        return;
    }

    TError::clear();
    int btState = atoi(pars[0].c_str());
    string text;

    // Because UTF8 is not supported out of the box from Windows and NetLinx
    // Studio has no native support for it, any UTF8 text must be encoded in
    // bytes. Because of this we must decode the bytes into real bytes here.
    if (pars.size() > 1)
    {
        string byte;
        size_t pos = 0;

        while (pos < pars[1].length())
        {
            byte = pars[1].substr(pos, 2);
            char ch = (char)strtol(byte.c_str(), NULL, 16);
            text += ch;
            pos += 2;
        }
    }

    vector<MAP_T> map = findButtons(port, channels);

    if (TError::isError() || map.empty())
        return;

    vector<Button::TButton *> buttons = collectButtons(map);

    if (buttons.size() > 0)
    {
        vector<Button::TButton *>::iterator mapIter;

        for (mapIter = buttons.begin(); mapIter != buttons.end(); mapIter++)
        {
            Button::TButton *bt = *mapIter;
            setButtonCallbacks(bt);

            if (btState == 0)       // All instances?
            {
                int bst = bt->getNumberInstances();
                MSG_DEBUG("Setting UNI on all " << bst << " instances...");

                for (int i = 0; i < bst; i++)
                    bt->setText(text, i);
            }
            else
                bt->setText(text, btState - 1);
        }
    }
}

void TPageManager::doUTF(int port, vector<int>& channels, vector<string>& pars)
{
    DECL_TRACER("TPageManager::doTXT(int port, vector<int>& channels, vector<string>& pars)");

    if (pars.size() < 1)
    {
        MSG_ERROR("Expecting 1 parameters but got none! Ignoring command.");
        return;
    }

    TError::clear();
    int btState = atoi(pars[0].c_str());
    string text;

    if (pars.size() > 1)
        text = pars[1];

    vector<MAP_T> map = findButtons(port, channels);

    if (TError::isError() || map.empty())
        return;

    vector<Button::TButton *> buttons = collectButtons(map);

    if (buttons.size() > 0)
    {
        vector<Button::TButton *>::iterator mapIter;

        for (mapIter = buttons.begin(); mapIter != buttons.end(); mapIter++)
        {
            Button::TButton *bt = *mapIter;
            setButtonCallbacks(bt);

            if (btState == 0)       // All instances?
            {
                int bst = bt->getNumberInstances();
                MSG_DEBUG("Setting TXT on all " << bst << " instances...");

                for (int i = 0; i < bst; i++)
                    bt->setText(text, i);
            }
            else
                bt->setText(text, btState - 1);
        }
    }
}

/**
 * Set the keyboard passthru.
 */
void TPageManager::doKPS(int, vector<int>&, vector<string>& pars)
{
    DECL_TRACER("TPageManager::doKPS(int, vector<int>&, vector<string>& pars)");

    if (pars.size() < 1)
    {
        MSG_ERROR("Got no parameter. Ignoring command!");
        return;
    }

    int state = atoi(pars[0].c_str());

    if (state == 0)
        mPassThrough = false;
    else if (state == 5)
        mPassThrough = true;
}

void TPageManager::doVKS(int, std::vector<int>&, vector<string>& pars)
{
    DECL_TRACER("TPageManager::doVKS(int, std::vector<int>&, vector<string>& pars)");

    if (pars.size() < 1)
    {
        MSG_ERROR("Got no parameter. Ignoring command!");
        return;
    }

    if (_sendVirtualKeys)
        _sendVirtualKeys(pars[0]);
}

/*
 * Set the bitmap of a button to use a particular resource.
 * Syntax:
 *    "'^BBR-<vt addr range>,<button states range>,<resource name>'"
 * Variable:
 *    variable text address range = 1 - 4000.
 *    button states range = 1 - 256 for multi-state buttons (0 = All states, for General buttons 1 = Off state and 2 = On state).
 *    resource name = 1 - 50 ASCII characters.
 * Example:
 *    SEND_COMMAND Panel,"'^BBR-700,1,Sports_Image'"
 *    Sets the resource name of the button to ’Sports_Image’.
 */
void TPageManager::doBBR(int port, vector<int>& channels, vector<string>& pars)
{
    DECL_TRACER("TPageManager::doBBR(int port, vector<int>& channels, vector<string>& pars)");

    if (pars.size() < 2)
    {
        MSG_ERROR("Expecting 2 parameters but got none! Ignoring command.");
        return;
    }

    TError::clear();
    int btState = atoi(pars[0].c_str());
    string resName = pars[1];

    vector<MAP_T> map = findButtons(port, channels);

    if (TError::isError() || map.empty())
        return;

    vector<Button::TButton *> buttons = collectButtons(map);

    if (buttons.size() > 0)
    {
        vector<Button::TButton *>::iterator mapIter;

        for (mapIter = buttons.begin(); mapIter != buttons.end(); mapIter++)
        {
            Button::TButton *bt = *mapIter;
            setButtonCallbacks(bt);

            if (btState == 0)       // All instances?
            {
                int bst = bt->getNumberInstances();
                MSG_DEBUG("Setting BBR on all " << bst << " instances...");

                for (int i = 0; i < bst; i++)
                    bt->setResourceName(resName, i);
            }
            else
                bt->setResourceName(resName, btState - 1);

            if (bt->isVisible())
                bt->refresh();
            else if (_setVisible)
                _setVisible(bt->getHandle(), false);
        }
    }
}

/*
 * Add new resources
 * Adds any and all resource parameters by sending embedded codes and data.
 * Since the embedded codes are preceded by a '%' character, any '%' character
 * contained in* the URL must be escaped with a second '%' character (see
 * example).
 * The file name field (indicated by a %F embedded code) may contain special
 * escape sequences as shown in the ^RAF, ^RMF.
 * Syntax:
 *    "'^RAF-<resource name>,<data>'"
 * Variables:
 *    resource name = 1 - 50 ASCII characters.
 *    data = Refers to the embedded codes, see the ^RAF, ^RMF.
 * Example:
 *    SEND_COMMAND Panel,"'^RAF-New Image,%P0%HAMX.COM%ALab/Test%%5Ffile%Ftest.jpg'"
 *    Adds a new resource.
 *    The resource name is ’New Image’
 *    %P (protocol) is an HTTP
 *    %H (host name) is AMX.COM
 *    %A (file path) is Lab/Test_f ile
 *    %F (file name) is test.jpg.
 *    Note that the %%5F in the file path is actually encoded as %5F.
 */
void TPageManager::doRAF(int, vector<int>&, vector<string>& pars)
{
    DECL_TRACER("TPageManager::doRAF(int port, vector<int>& channels, vector<string>& pars)");

    if (pars.size() < 2)
    {
        MSG_ERROR("Expecting 2 parameters but got none! Ignoring command.");
        return;
    }

    string name = pars[0];
    string data = pars[1];

    vector<string> parts = StrSplit(data, "%");
    RESOURCE_T res;

    if (parts.size() > 0)
    {
        vector<string>::iterator sIter;

        for (sIter = parts.begin(); sIter != parts.end(); sIter++)
        {
            const char *s = sIter->c_str();
            string ss = *sIter;
            MSG_DEBUG("Parsing \"" << ss << "\" with token << " << ss[0]);

            switch(*s)
            {
                case 'P':
                    if (*(s+1) == '0')
                        res.protocol = "HTTP";
                    else
                        res.protocol = "FTP";
                    break;

                case 'U': res.user = sIter->substr(1); break;
                case 'S': res.password = sIter->substr(1); break;
                case 'H': res.host = sIter->substr(1); break;
                case 'F': res.file = sIter->substr(1); break;
                case 'A': res.path = sIter->substr(1); break;
                case 'R': res.refresh = atoi(sIter->substr(1).c_str()); break;

                default:
                    MSG_WARNING("Option " << sIter->at(0) << " is currently not implemented!");
            }
        }

        if (gPrjResources)
            gPrjResources->addResource(name, res.protocol, res.host, res.path, res.file, res.user, res.password, res.refresh);
    }
}

void TPageManager::doRFR(int, vector<int>&, vector<string>& pars)
{
    DECL_TRACER("TPageManager::doRFR(int port, vector<int>& channels, vector<string>& pars)");

    if (pars.size() < 1)
    {
        MSG_ERROR("Expecting 1 parameter but got none! Ignoring command.");
        return;
    }

    string name = pars[0];
    vector<MAP_T> map = findButtonByName(name);

    if (TError::isError() || map.empty())
        return;

    vector<Button::TButton *> buttons = collectButtons(map);

    if (buttons.size() > 0)
    {
        vector<Button::TButton *>::iterator mapIter;

        for (mapIter = buttons.begin(); mapIter != buttons.end(); mapIter++)
        {
            Button::TButton *bt = *mapIter;

            if (bt->isVisible())
            {
                setButtonCallbacks(bt);
                bt->refresh();
            }
        }
    }
}

/*
 * Modify an existing resource
 *
 * Modifies any and all resource parameters by sending embedded codes and data.
 * Since the embedded codes are preceded by a '%' character, any '%' character
 * contained in the URL must be escaped with a second '%' character (see
 * example).
 * The file name field (indicated by a %F embedded code) may contain special
 * escape sequences as shown in the ^RAF.
 *
 * Syntax:
 * "'^RMF-<resource name>,<data>'"
 * Variables:
 *   • resource name = 1 - 50 ASCII characters
 *   • data = Refers to the embedded codes, see the ^RAF, ^RMF.
 * Example:
 *   SEND_COMMAND Panel,"'^RMF-Sports_Image,%ALab%%5FTest/Images%Ftest.jpg'"
 * Changes the resource ’Sports_Image’ file name to ’test.jpg’ and the path to
 * ’Lab_Test/Images’.
 * Note that the %%5F in the file path is actually encoded as %5F.
 */
void TPageManager::doRMF(int, vector<int>&, vector<string>& pars)
{
    DECL_TRACER("TPageManager::doRMF(int port, vector<int>& channels, vector<string>& pars)");

    if (pars.size() < 2)
    {
        MSG_ERROR("Expecting 2 parameters but got none! Ignoring command.");
        return;
    }

    string name = pars[0];
    string data = pars[1];

    vector<string> parts = StrSplit(data, "%");
    RESOURCE_T res;

    if (parts.size() > 0)
    {
        vector<string>::iterator sIter;

        for (sIter = parts.begin(); sIter != parts.end(); sIter++)
        {
            const char *s = sIter->c_str();
            string ss = *sIter;
            MSG_DEBUG("Parsing \"" << ss << "\" with token << " << ss[0]);

            switch(*s)
            {
                case 'P':
                    if (*(s+1) == '0')
                        res.protocol = "HTTP";
                    else
                        res.protocol = "FTP";
                break;

                case 'U': res.user = sIter->substr(1); break;
                case 'S': res.password = sIter->substr(1); break;
                case 'H': res.host = sIter->substr(1); break;
                case 'F': res.file = sIter->substr(1); break;
                case 'A': res.path = sIter->substr(1); break;
                case 'R': res.refresh = atoi(sIter->substr(1).c_str()); break;

                default:
                    MSG_WARNING("Option " << sIter->at(0) << " is currently not implemented!");
            }
        }

        if (gPrjResources)
            gPrjResources->setResource(name, res.protocol, res.host, res.path, res.file, res.user, res.password, res.refresh);
    }
}

/**
 * Change the refresh rate for a given resource.
 */
void TPageManager::doRSR(int, vector<int>&, vector<string>& pars)
{
    DECL_TRACER("TPageManager::doRSR(int, vector<int>&, vector<string>& pars)");

    if (pars.size() < 2)
    {
        MSG_ERROR("Expecting 2 parameters but got none! Ignoring command.");
        return;
    }

    string resName = pars[0];
    int resRefresh = atoi(pars[1].c_str());

    if (!gPrjResources)
    {
        MSG_ERROR("Missing the resource module. Ignoring command!");
        return;
    }

    RESOURCE_T res = gPrjResources->findResource(resName);

    if (res.name.empty() || res.refresh == resRefresh)
        return;

    gPrjResources->setResource(resName, res.protocol, res.host, res.path, res.file, res.user, res.password, resRefresh);
}

/**
 * @brief TPageManager::doAKB - Pop up the keyboard icon
 * Pop up the keyboard icon and initialize the text string to that specified.
 * Keyboard string is set to null on power up and is stored until power is lost.
 * The Prompt Text is optional.
 */
void TPageManager::doAKB(int, vector<int>&, vector<string> &pars)
{
    DECL_TRACER("TPageManager::doAKB(int, vector<int>&, vector<string> &pars)");

    if (pars.size() < 1)
    {
        MSG_ERROR("Expecting 2 parameters but got only " << pars.size() << "! Ignoring command.");
        return;
    }

    string initText = pars[0];
    string promptText;

    if (pars.size() > 1)
        promptText = pars[1];

    if (initText.empty())
        initText = mAkbText;
    else
        mAkbText = initText;

    if (_callKeyboard)
        _callKeyboard(initText, promptText, false);
}

/**
 * Pop up the keyboard icon and initialize the text string to that
 * specified.
 */
void TPageManager::doAKEYB(int port, vector<int>& channels, vector<string>& pars)
{
    DECL_TRACER("TPageManager::doAKEYB(int port, vector<int>& channels, vector<string>& pars)");

    doAKB(port, channels, pars);
}

void TPageManager::doAKEYP(int port, std::vector<int>& channels, std::vector<std::string>& pars)
{
    DECL_TRACER("TPageManager::doAKEYP(int port, std::vector<int>& channels, std::vector<std::string>& pars)");

    doAKP(port, channels, pars);
}

/**
 * Remove keyboard or keypad that was displayed using 'AKEYB', 'AKEYP', 'PKEYP',
 * @AKB, @AKP, @PKP, @EKP, or @TKP commands.
 */
void TPageManager::doAKEYR(int, vector<int>&, vector<string>&)
{
    DECL_TRACER("TPageManager::doAKEYR(int, vector<int>&, vector<string>&)");

    if (_callResetKeyboard)
        _callResetKeyboard();
}

/**
 * @brief TPageManager::doAKP - Pop up the keypad icon
 * Pop up the keypad icon and initialize the text string to that specified.
 * Keypad string is set to null on power up and is stored until power is lost.
 * The Prompt Text is optional.
 */
void TPageManager::doAKP(int, std::vector<int>&, std::vector<std::string> &pars)
{
    DECL_TRACER("TPageManager::doAKP(int, vector<int>&, vector<string> &pars)");

    if (pars.size() < 1)
    {
        MSG_ERROR("Expecting 2 parameters but got only " << pars.size() << "! Ignoring command.");
        return;
    }

    string initText = pars[0];
    string promptText;

    if (pars.size() > 1)
        promptText = pars[1];

    if (initText.empty())
        initText = mAkpText;
    else
        mAkpText = initText;

    if (_callKeypad)
        _callKeypad(initText, promptText, false);
}

/**
 * Remove keyboard or keypad that was displayed using 'AKEYB', 'AKEYP', 'PKEYP',
 * @AKB, @AKP, @PKP, @EKP, or @TKP commands.
 */
void TPageManager::doAKR(int port, vector<int>& channels, vector<string>& pars)
{
    DECL_TRACER("TPageManager::doAKR(int, vector<int>&, vector<string>&)");

    doAKEYR(port, channels, pars);
}

void TPageManager::doABEEP(int, std::vector<int>&, vector<string>&)
{
    DECL_TRACER("TPageManager::doBEEP(int, std::vector<int>&, vector<string>&)");

    if (!_playSound)
        return;

    string snd = TConfig::getSystemPath(TConfig::SOUNDS) + "/" + TConfig::getSingleBeepSound();
    TValidateFile vf;

    if (_playSound && vf.isValidFile(snd))
        _playSound(snd);
}

void TPageManager::doADBEEP(int, std::vector<int>&, vector<string>&)
{
    DECL_TRACER("TPageManager::doDBEEP(int, std::vector<int>&, vector<string>&)");

    if (!_playSound)
        return;

    string snd = TConfig::getSystemPath(TConfig::SOUNDS) + "/" + TConfig::getDoubleBeepSound();
    TValidateFile vf;

    if (_playSound && vf.isValidFile(snd))
        _playSound(snd);
}

void TPageManager::doBEEP(int, std::vector<int>&, vector<string>&)
{
    DECL_TRACER("TPageManager::doBEEP(int, std::vector<int>&, vector<string>&)");

    if (!_playSound)
        return;

    string snd = TConfig::getSystemPath(TConfig::SOUNDS) + "/" + TConfig::getSingleBeepSound();
    TValidateFile vf;
    TSystemSound sysSound(TConfig::getSystemPath(TConfig::SOUNDS));

    if (_playSound && sysSound.getSystemSoundState() && vf.isValidFile(snd))
        _playSound(snd);
}

void TPageManager::doDBEEP(int, std::vector<int>&, vector<string>&)
{
    DECL_TRACER("TPageManager::doDBEEP(int, std::vector<int>&, vector<string>&)");

    if (!_playSound)
        return;

    string snd = TConfig::getSystemPath(TConfig::SOUNDS) + "/" + TConfig::getDoubleBeepSound();
    TValidateFile vf;
    TSystemSound sysSound(TConfig::getSystemPath(TConfig::SOUNDS));

    if (_playSound && sysSound.getSystemSoundState() && vf.isValidFile(snd))
        _playSound(snd);
}

/**
 * @brief Pop up the keypad icon and initialize the text string to that specified.
 * Keypad string is set to null on power up and is stored until power is lost.
 * The Prompt Text is optional.
 */
void TPageManager::doEKP(int port, std::vector<int>& channels, std::vector<std::string>& pars)
{
    DECL_TRACER("TPageManager::doEKP(int port, std::vector<int>& channels, std::vector<std::string>& pars)");

    doAKP(port, channels, pars);
}

/**
 * @brief Present a private keyboard.
 * Pops up the keyboard icon and initializes the text string to that specified.
 * Keyboard displays a '*' instead of the letters typed. The Prompt Text is optional.
 */
void TPageManager::doPKB(int, vector<int>&, vector<string>& pars)
{
    DECL_TRACER("TPageManager::doPKB(int, vector<int>&, vector<string>& pars)");

    if (pars.size() < 1)
    {
        MSG_ERROR("Expecting 2 parameters but got only " << pars.size() << "! Ignoring command.");
        return;
    }

    string initText = pars[0];
    string promptText;

    if (pars.size() > 1)
        promptText = pars[1];

    if (_callKeyboard)
        _callKeyboard(initText, promptText, true);
}

/**
 * @brief Present a private keypad.
 * Pops up the keypad icon and initializes the text string to that specified.
 * Keypad displays a '*' instead of the numbers typed. The Prompt Text is optional.
 */
void TPageManager::doPKP(int, vector<int>&, vector<string>& pars)
{
    DECL_TRACER("TPageManager::doPKP(int, vector<int>&, vector<string>& pars)");

    if (pars.size() < 1)
    {
        MSG_ERROR("Expecting 2 parameters but got only " << pars.size() << "! Ignoring command.");
        return;
    }

    string initText = pars[0];
    string promptText;

    if (pars.size() > 1)
        promptText = pars[1];

    if (_callKeypad)
        _callKeypad(initText, promptText, true);
}

/**
 * Send panel to SETUP page.
 */
void TPageManager::doSetup(int, vector<int>&, vector<string>&)
{
    DECL_TRACER("TPageManager::doSetup(int, vector<int>&, vector<string>&)");

    MSG_PROTOCOL("Received command to show setup ...");

    if (_callShowSetup)
        _callShowSetup();
}

/**
 * Shut down the App
 */
void TPageManager::doShutdown(int, vector<int>&, vector<string>&)
{
    DECL_TRACER("TPageManager::doShutdown(int, vector<int>&, vector<string>&)");

    MSG_PROTOCOL("Received shutdown ...");
#ifdef __ANDROID__
    stopNetworkState();
#endif
    prg_stopped = true;
    killed = true;

    if (_shutdown)
        _shutdown();
}

void TPageManager::doSOU(int, vector<int>&, vector<string>& pars)
{
    DECL_TRACER("TPageManager::doSOU(int, vector<int>&, vector<string>& pars)");

    if (pars.size() < 1)
    {
        MSG_ERROR("@SOU: Expecting a sound file as parameter! Ignoring command.");
        return;
    }

    if (!_playSound)
    {
        MSG_ERROR("@SOU: Missing sound module!");
        return;
    }

    _playSound(pars[0]);
}

/**
 * @brief Present a telephone keypad.
 * Pops up the keypad icon and initializes the text string to that specified.
 * The Prompt Text is optional.
 */
void TPageManager::doTKP(int port, vector<int>& channels, vector<string>& pars)
{
    DECL_TRACER("TPageManager::doTKP(int port, vector<int>& channels, vector<string>& pars)");

    // TODO: Implement a real telefone keypad.
    doAKP(port, channels, pars);
}

/**
 * Popup the virtual keyboard
 */
void TPageManager::doVKB(int port, vector<int>& channels, vector<string>& pars)
{
    DECL_TRACER("TPageManager::doVKB(int port, vector<int>& channels, vector<string>& pars)");

    doAKP(port, channels, pars);
}
#ifndef _NOSIP_
void TPageManager::sendPHN(vector<string>& cmds)
{
    DECL_TRACER("TPageManager::sendPHN(const vector<string>& cmds)");

    vector<int> channels;
    doPHN(-1, channels, cmds);
}

/**
 * @brief Phone commands.
 * The phone commands could come from the master or are send to the master.
 * If the parameter \p port is less then 0 (zero) a command is send to the
 * master. In any other case the command came from the mater.
 *
 * @param port  This is used to signal if the command was sent by the master
 *              or generated from the panel. If ths is less then 0, then the
 *              method was called because of an event happen in the panel.
 *              If this is grater or equal 0, then the event is comming from
 *              the master.
 * @param pars  This are parameters. The first parameter defines the action
 *              to be done. According to the command this parameter may have a
 *              different number of arguments.
 */
void TPageManager::doPHN(int port, vector<int>&, vector<string>& pars)
{
    DECL_TRACER("TPageManager::doPHN(int port, vector<int>&, vector<string>& pars)");

    if (pars.size() < 1)
    {
        MSG_ERROR("Expecting at least 1 parameter but got none! Ignoring command.");
        return;
    }

    string sCommand;
    string cmd = toUpper(pars[0]);

    // Master to panel
    if (port >= 0)
    {
        if (!mSIPClient)
        {
            MSG_ERROR("SIP client class was not initialized!")
            return;
        }

        if (cmd == "ANSWER")
        {
            if (pars.size() >= 2)
            {
                int id = atoi(pars[1].c_str());

                if (mSIPClient->getSIPState(id) == TSIPClient::SIP_HOLD)
                    mSIPClient->resume(id);
                else
                    mSIPClient->pickup(nullptr, id);
            }
        }
        else if (cmd == "AUTOANSWER")
        {
            if (pars.size() >= 2)
            {
                if (pars[1].at(0) == '0')
                    mPHNautoanswer = false;
                else
                    mPHNautoanswer = true;

                vector<string> cmds;
                cmds = { "AUTOANSWER", to_string(mPHNautoanswer ? 1 : 0) };
                sendPHN(cmds);
            }
        }
        else if (cmd == "CALL")     // Initiate a call
        {
            if (pars.size() >= 2)
                mSIPClient->call(pars[1]);
        }
        else if (cmd == "DTMF")     // Send tone modified codes
        {
            if (pars.size() >= 2)
                mSIPClient->sendDTMF(pars[1]);
        }
        else if (cmd == "HANGUP")   // terminate a call
        {
            if (pars.size() >= 2)
            {
                int id = atoi(pars[1].c_str());
                mSIPClient->terminate(id);
            }
        }
        else if (cmd == "HOLD")     // Hold the line
        {
            if (pars.size() >= 2)
            {
                int id = atoi(pars[1].c_str());
                mSIPClient->hold(id);
            }
        }
        else if (cmd == "LINESTATE") // State of all line
        {
            mSIPClient->sendLinestate();
        }
        else if (cmd == "PRIVACY")  // Set/unset "do not disturb"
        {
            if (pars.size() >= 2)
            {
                bool state = (pars[1].at(0) == '1' ? true : false);
                mSIPClient->sendPrivate(state);
            }
        }
        else if (cmd == "REDIAL")   // Redials the last number
        {
            mSIPClient->redial();
        }
        else if (cmd == "TRANSFER") // Transfer call to provided number
        {
            if (pars.size() >= 3)
            {
                int id = atoi(pars[1].c_str());
                string num = pars[2];

                if (mSIPClient->transfer(id, num))
                {
                    vector<string> cmds;
                    cmds.push_back("TRANSFERRED");
                    sendPHN(cmds);
                }
            }
        }
        else if (cmd == "SETUP")    // Some temporary settings
        {
            if (pars.size() < 2)
                return;

            if (pars[1] == "DOMAIN" && pars.size() >= 3)
                TConfig::setSIPdomain(pars[2]);
            else if (pars[1] == "DTMFDURATION")
            {
                // FIXME: How to do this?
                MSG_WARNING("Setting the DTMF duration is currently not implemented.");
            }
            else if (pars[1] == "ENABLE")   // (re)register user
            {
                TConfig::setSIPstatus(true);
                mSIPClient->cleanUp();
                mSIPClient->connectSIPProxy();
            }
            else if (pars[1] == "DOMAIN" && pars.size() >= 3)
                TConfig::setSIPdomain(pars[2]);
            else if (pars[1] == "PASSWORD" && pars.size() >= 3)
                TConfig::setSIPpassword(pars[2]);
            else if (pars[1] == "PORT" && pars.size() != 3)
                TConfig::setSIPport(atoi(pars[2].c_str()));
            else if (pars[1] == "PROXYADDR" && pars.size() >= 3)
                TConfig::setSIPproxy(pars[2]);
            else if (pars[1] == "STUNADDR" && pars.size() >= 3)
                TConfig::setSIPstun(pars[2]);
            else if (pars[1] == "USERNAME" && pars.size() >= 3)
                TConfig::setSIPuser(pars[2]);
        }
        else
        {
            MSG_ERROR("Unknown command ^PHN-" << cmd << " ignored!");
        }
    }
    else   // Panel to master
    {
        vector<string>::iterator iter;

        for (iter = pars.begin(); iter != pars.end(); ++iter)
        {
            if (!sCommand.empty())
                sCommand += ",";

            sCommand += *iter;
        }

        sendPHNcommand(sCommand);
    }
}

void TPageManager::getPHN(int, vector<int>&, vector<string>& pars)
{
    DECL_TRACER("TPageManager::getPHN(int, vector<int>&, vector<string>& pars)");

    if (pars.size() < 1)
    {
        MSG_ERROR("Invalid number of arguments!");
        return;
    }

    string cmd = pars[0];

    if (cmd == "AUTOANSWER")
        sendPHNcommand(cmd + "," + (mPHNautoanswer ? "1" : "0"));
    else if (cmd == "LINESTATE")
    {
        TSIPClient::SIP_STATE_t state1 = TSIPClient::SIP_NONE;
        TSIPClient::SIP_STATE_t state2 = TSIPClient::SIP_NONE;

        if (!mSIPClient)
            return;

        state1 = mSIPClient->getSIPState(0);
        state2 = mSIPClient->getSIPState(1);

        sendPHNcommand(cmd + ",0," + sipStateToString(state1) + ",1," + sipStateToString(state2));
    }
    else if (cmd == "MSGWAITING")
    {
        // Currently messages are not supported!
        sendPHNcommand(cmd + ",0");
    }
    else if (cmd == "PRIVACY")
    {
        // FIXME
        sendPHNcommand(cmd + ",0");
    }
    else
    {
        MSG_WARNING("Unknown command " << cmd << " found!");
    }
}
#endif  // _NOSIP_
