/***************************************************************************
 *   Copyright (C) 2017 by Nicolas Carion                                  *
 *   This file is part of Kdenlive. See www.kdenlive.org.                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) version 3 or any later version accepted by the       *
 *   membership of KDE e.V. (or its successor approved  by the membership  *
 *   of KDE e.V.), which shall act as a proxy defined in Section 14 of     *
 *   version 3 of the license.                                             *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 ***************************************************************************/

#include "profilerepository.hpp"
#include "profilemodel.hpp"
#include "kdenlive_debug.h"
#include "kdenlivesettings.h"
#include <QDir>
#include <QStandardPaths>
#include <mlt++/MltProfile.h>


std::unique_ptr<ProfileRepository> ProfileRepository::instance;
std::once_flag ProfileRepository::m_onceFlag;

ProfileRepository::ProfileRepository()
{
    refresh();
}

std::unique_ptr<ProfileRepository> & ProfileRepository::get()
{
    std::call_once(m_onceFlag, []{instance.reset(new ProfileRepository());});
    return instance;
}

void ProfileRepository::refresh()
{
    QWriteLocker locker(&m_mutex);

    m_profiles.clear();

    //Helper function to check a profile and print debug info
    auto check_profile = [&](std::unique_ptr<ProfileModel>& profile, const QString& file) {
        if (m_profiles.count(file) > 0) {
            qCWarning(KDENLIVE_LOG) << "//// Duplicate profile found: "<<file<<". Ignoring.";
            return false;
        }
        if (!profile->is_valid()) {
            qCWarning(KDENLIVE_LOG) << "//// WARNING: invalid profile found: "<<file<<". Ignoring.";
            return false;
        }
        return true;
    };

    //list MLT profiles.
    QDir mltDir(KdenliveSettings::mltpath());
    QStringList profilesFiles = mltDir.entryList(QDir::Files);

    //list Custom Profiles
    QStringList customProfilesDir = QStandardPaths::locateAll(QStandardPaths::AppDataLocation, QStringLiteral("profiles/"), QStandardPaths::LocateDirectory);
    for (const auto& dir : customProfilesDir) {
        QStringList files = QDir(dir).entryList(QDir::Files);
        for (const auto& file : files) {
            profilesFiles << QDir(dir).absoluteFilePath(file);
        }
    }

    qDebug() << "all profiles "<<profilesFiles;
    //Iterate through files
    for (const auto& file : profilesFiles) {
        std::unique_ptr<ProfileModel> profile(new ProfileModel(file));
        if (check_profile(profile, file)) {
            m_profiles.insert(std::make_pair(file, std::move(profile)));
        }
    }

}


QVector<QPair<QString, QString> > ProfileRepository::getAllProfiles()
{
    QReadLocker locker(&m_mutex);

    QVector<QPair<QString, QString> > list;
    for (const auto& profile : m_profiles) {
        list.push_back({profile.second->description(), profile.first});
    }
    qSort(list);
    return list;
}