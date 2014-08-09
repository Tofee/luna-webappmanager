/*
 * Copyright (C) 2013 Simon Busch <morphis@gravedo.de>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>
 */

#include <QDebug>
#include <QStringList>
#include <QTime>
#include <QtGlobal>

#include <glib.h>
#include <systemd/sd-daemon.h>

#include <LocalePreferences.h>

#include "webappmanager.h"
#include "systemtime.h"

#define VERSION "0.1"
#define XDG_RUNTIME_DIR_DEFAULT "/tmp/luna-session"

static gboolean option_version = FALSE;
static gboolean option_verbose = FALSE;
static gboolean option_systemd = FALSE;

static GOptionEntry options[] = {
    { "verbose", 0, 0, G_OPTION_ARG_NONE, &option_verbose, "Enable verbose logging" },
    { "version", 'v', 0, G_OPTION_ARG_NONE, &option_version,
        "Show version information and exit" },
    { "systemd", 0, 0, G_OPTION_ARG_NONE, &option_systemd, "Start with systemd support" },
    { NULL },
};

void messageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    QString timeStr = QTime::currentTime().toString("hh:mm:ss.zzz");

    switch(type)
    {
    case QtDebugMsg:
        if (option_verbose)
            fprintf(stdout, "DEBUG: %s: %s\n", timeStr.toUtf8().constData(), msg.toUtf8().constData());
        break;
    case QtWarningMsg:
        fprintf(stdout, "WARNING: %s: %s\n", timeStr.toUtf8().constData(), msg.toUtf8().constData());
        break;
    case QtCriticalMsg:
        fprintf(stdout, "CRITICAL: %s: %s\n", timeStr.toUtf8().constData(), msg.toUtf8().constData());
        break;
    case QtFatalMsg:
        fprintf(stdout, "FATAL: %s: %s\n", timeStr.toUtf8().constData(), msg.toUtf8().constData());
        break;
    default:
        fprintf(stdout, "INFO: %s: %s\n", timeStr.toUtf8().constData(), msg.toUtf8().constData());
        break;
    }
}

int main(int argc, char **argv)
{
    GError *error = NULL;
    GOptionContext *context;

    qInstallMessageHandler(messageHandler);

    if (qgetenv("DISPLAY").isEmpty()) {
        setenv("EGL_PLATFORM", "wayland", 0);
        setenv("QT_QPA_PLATFORM", "wayland", 0);
        setenv("XDG_RUNTIME_DIR", XDG_RUNTIME_DIR_DEFAULT, 0);
        setenv("QT_IM_MODULE", "Maliit", 0);
        setenv("QT_WAYLAND_DISABLE_WINDOWDECORATION", "1", 1);
    }

    QString storagePath = "/media/cryptofs/.sysmgr";
    QString storagePathEnv = qgetenv("PERSISTENT_STORAGE_PATH");
    if (!storagePathEnv.isEmpty())
        storagePath = storagePathEnv;

    if (qgetenv("XDG_DATA_HOME").isEmpty()) {
        QString dataDir = QString("%1/data").arg(storagePath);
        setenv("XDG_DATA_HOME", dataDir.toUtf8().constData(), 1);
    }

    if (qgetenv("XDG_CACHE_HOME").isEmpty()) {
        QString cacheDir = QString("%1/cache").arg(storagePath);
        setenv("XDG_CACHE_HOME", cacheDir.toUtf8().constData(), 1);
    }

    luna::WebAppManager webAppManager(argc, argv);

    context = g_option_context_new(NULL);
    g_option_context_add_main_entries(context, options, NULL);

    if (!g_option_context_parse(context, &argc, &argv, &error)) {
        if (error) {
            g_printerr("%s\n", error->message);
            g_error_free(error);
        }
        else
            g_printerr("An unknown error occurred\n");
        exit(1);
    }

    g_option_context_free(context);

    if (option_version) {
        g_message("LunaWebAppMgr %s", VERSION);
        goto cleanup;
    }

#if 0
    if (option_debug)
        setenv("QTWEBKIT_INSPECTOR_SERVER", "1122", 0);
#endif

    LocalePreferences::instance();
    luna::SystemTime::instance();

    if (option_systemd)
        sd_notify(0, "READY=1");

    webAppManager.exec();

cleanup:

    return 0;
}
