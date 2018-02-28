/* XMRig
 * Copyright 2010      Jeff Garzik <jgarzik@pobox.com>
 * Copyright 2012-2014 pooler      <pooler@litecoinpool.org>
 * Copyright 2014      Lucas Jones <https://github.com/lucasjones>
 * Copyright 2014-2016 Wolf9466    <https://github.com/OhGodAPet>
 * Copyright 2016      Jay D Dee   <jayddee246@gmail.com>
 * Copyright 2016-2018 XMRig       <support@xmrig.com>
 *
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program. If not, see <http://www.gnu.org/licenses/>.
 */


#include <stdio.h>


#include "core/Config.h"
#include "core/ConfigLoader.h"
#include "core/ConfigWatcher.h"
#include "interfaces/IWatcherListener.h"
#include "log/Log.h"


xmrig::ConfigWatcher::ConfigWatcher(const char *path, IWatcherListener *listener) :
    m_path(strdup(path)),
    m_listener(listener)
{
    uv_fs_event_init(uv_default_loop(), &m_fsEvent);
    uv_timer_init(uv_default_loop(), &m_timer);

    m_fsEvent.data = m_timer.data = this;

    start();
}


xmrig::ConfigWatcher::~ConfigWatcher()
{
    uv_timer_stop(&m_timer);
    uv_fs_event_stop(&m_fsEvent);

    free(m_path);
}


void xmrig::ConfigWatcher::onTimer(uv_timer_t* handle)
{
    static_cast<xmrig::ConfigWatcher *>(handle->data)->reload();
}


void xmrig::ConfigWatcher::onFsEvent(uv_fs_event_t* handle, const char *filename, int events, int status)
{
    if (!filename) {
        return;
    }

    static_cast<xmrig::ConfigWatcher *>(handle->data)->queueUpdate();
}


void xmrig::ConfigWatcher::queueUpdate()
{
    uv_timer_stop(&m_timer);
    uv_timer_start(&m_timer, xmrig::ConfigWatcher::onTimer, kDelay, 0);
}


void xmrig::ConfigWatcher::reload()
{
    LOG_WARN("\"%s\" was changed, reloading configuration", m_path);

    xmrig::Config *config = new xmrig::Config();
    ConfigLoader::parseConfig(config, m_path);

    if (!config->isValid()) {
        LOG_ERR("reloading failed");

        delete config;
        return;
    }

    m_listener->onNewConfig(config);

    uv_fs_event_stop(&m_fsEvent);
    start();
}


void xmrig::ConfigWatcher::start()
{
    uv_fs_event_start(&m_fsEvent, xmrig::ConfigWatcher::onFsEvent, m_path, 0);
}
