﻿#include "notif.h"

// global
NotificationBuffer g_notificationBuffer;

std::string Notification::getTypeStr(ServMgr::NOTIFY_TYPE type)
{
    switch (type)
    {
    case ServMgr::NT_UPGRADE:
        return "Upgrade Alert";
    case ServMgr::NT_PEERCAST:
        return "Peercast";
    case ServMgr::NT_BROADCASTERS:
        return "Broadcasters";
    case ServMgr::NT_TRACKINFO:
        return "Track Info";
    default:
        return "Unknown";
    }
}

NotificationBuffer::NotificationBuffer()
{
}

int NotificationBuffer::numUnread()
{
    std::lock_guard<std::recursive_mutex> cs(lock);

    int count = 0;
    for (auto& e : notifications)
    {
        count += !e.isRead;
    }
    return count;
}

void NotificationBuffer::markAsRead(unsigned int ctime)
{
    std::lock_guard<std::recursive_mutex> cs(lock);

    for (auto& e : notifications)
    {
        if (e.notif.time <= ctime)
            e.isRead = true;
    }
}

int NotificationBuffer::numNotifications()
{
    std::lock_guard<std::recursive_mutex> cs(lock);

    return static_cast<int>(notifications.size());
}

NotificationBuffer::Entry NotificationBuffer::getNotification(int index)
{
    std::lock_guard<std::recursive_mutex> cs(lock);

    try
    {
        return notifications.at(index);
    } catch (std::out_of_range&)
    {
        return Entry(Notification(), false);
    }
}

void NotificationBuffer::addNotification(const Notification& notif)
{
    std::lock_guard<std::recursive_mutex> cs(lock);

    while (notifications.size() >= MAX_NOTIFS)
        notifications.pop_back();

    notifications.push_front(Entry(notif, false));
}

bool NotificationBuffer::writeVariable(Stream& out, const String& varName)
{
    if (varName == "numNotifications") {
        out.writeString(std::to_string(numNotifications()).c_str());
        return true;
    } else if (varName == "numUnread") {
        out.writeString(std::to_string(numUnread()).c_str());
        return true;
    } else if (varName == "markAsRead") {
        markAsRead(sys->getTime());
        out.writeString("marked all notifications as read");
        return true;
    } else {
        return false;
    }
}

bool NotificationBuffer::writeVariable(Stream& out, const String& varName, int index)
{
    if (varName.startsWith("notification.")) {
        String prop = varName + strlen("notification.");
        Entry e = getNotification(index);
        if (prop == "message") {
            out.writeString(e.notif.message.c_str());
            return true;
        } else if (prop == "isRead") {
            out.writeString(std::to_string((int) e.isRead).c_str());
            return true;
        } else if (prop == "type") {
            out.writeString(e.notif.getTypeStr().c_str());
            return true;
        } else if (prop == "unixTime") {
            out.writeString(std::to_string(e.notif.time).c_str());
            return true;
        } else if (prop == "time") {
            String t;
            t.setFromTime(e.notif.time);
            if (t.cstr()[strlen(t)-1] == '\n')
                t.cstr()[strlen(t)-1] = '\0';
            out.writeString(t.cstr());
            return true;
        }
    }
    return false;
}
