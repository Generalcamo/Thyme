/**
 * @file
 *
 * @author Jonathan Wilson
 *
 * @brief
 *
 * @copyright Thyme is free software: you can redistribute it and/or
 *            modify it under the terms of the GNU General Public License
 *            as published by the Free Software Foundation, either version
 *            2 of the License, or (at your option) any later version.
 *            A full copy of the GNU General Public License can be found in
 *            LICENSE
 */
#include "thingfactory.h"
#include "gameclient.h"
#include <cstring>

#ifndef GAME_DLL
ThingFactory *g_theThingFactory = nullptr;
#endif

ThingFactory::ThingFactory() : m_firstTemplate(nullptr), m_nextTemplateID(1) {}

ThingFactory::~ThingFactory()
{
    Free_Database();
}

ThingTemplate *ThingFactory::New_Template(const Utf8String &name)
{
    ThingTemplate *new_template = NEW_POOL_OBJ(ThingTemplate);
    const ThingTemplate *default_template = Find_Template("DefaultThingTemplate", false);

    if (default_template != nullptr) {
        *new_template = *default_template;
        new_template->Set_Copied_From_Default();
    }

    new_template->Friend_Set_Template_ID(m_nextTemplateID);
    m_nextTemplateID++;

    if (m_nextTemplateID == 0) {
        captainslog_dbgassert(0, "m_nextTemplateID wrapped to zero");
    }

    new_template->Friend_Set_Template_Name(name);
    Add_Template(new_template);
    return new_template;
}

ThingTemplate *ThingFactory::New_Override(ThingTemplate *thing_template)
{
    captainslog_dbgassert(thing_template, "NULL 'parent' thing template");
    captainslog_dbgassert(Find_Template(thing_template->Get_Name(), true),
        "Thing template '%s' not in master list",
        thing_template->Get_Name().Str());

    ThingTemplate *new_template = NEW_POOL_OBJ(ThingTemplate);
    ThingTemplate *overrid = (ThingTemplate *)thing_template->Get_Final_Override();
    *new_template = *overrid;
    new_template->Set_Copied_From_Default();
    new_template->Set_Is_Allocated();
    overrid->Set_Next(new_template);
    return new_template;
}

void ThingFactory::Free_Database()
{
    while (m_firstTemplate != nullptr) {
        ThingTemplate *t = m_firstTemplate;
        m_firstTemplate = t->Friend_Get_Next_Template();
        t->Delete_Instance();
    }

    m_templateMap.clear();
}

void ThingFactory::PostProcessLoad()
{
    for (ThingTemplate *t = m_firstTemplate; t != nullptr; t = t->Friend_Get_Next_Template()) {
        t->Resolve_Names();
    }
}

void ThingFactory::Reset()
{
    ThingTemplate *t = m_firstTemplate;

    while (t) {
        bool first = false;
        ThingTemplate *next = t->Friend_Get_Next_Template();

        if (t == m_firstTemplate) {
            first = true;
        }

        Utf8String str(t->Get_Name());
        Overridable *o = t->Delete_Overrides();

        if (o == nullptr) {
            if (first) {
                m_firstTemplate = next;
            }
            m_templateMap.erase(str);
        }

        t = next;
    }
}

void ThingFactory::Add_Template(ThingTemplate *tmplate)
{
    if (m_templateMap.find(tmplate->Get_Name()) != m_templateMap.end()) {
        captainslog_dbgassert(0, "Duplicate Thing Template name found: %s", tmplate->Get_Name().Str());
    }

    tmplate->Friend_Set_Next_Template(m_firstTemplate);
    m_firstTemplate = tmplate;
    m_templateMap[tmplate->Get_Name()] = tmplate;
}

const ThingTemplate *ThingFactory::Find_Template_By_ID(unsigned short id)
{
    for (ThingTemplate *t = m_firstTemplate; t != nullptr; t = t->Friend_Get_Next_Template()) {
        if (t->Get_Template_ID() == id) {
            return t;
        }
    }

    captainslog_dbgassert(0, "template %d not found", id);
    return nullptr;
}

const ThingTemplate *ThingFactory::Find_Template_Internal(const Utf8String &name, bool b)
{
    auto i = m_templateMap.find(name);

    if (i != m_templateMap.end()) {
        return i->second;
    }

    if (!strncmp(name, "***TESTING", strlen("***TESTING"))) {
        ThingTemplate *tmplate = New_Template("Un-namedTemplate");
        tmplate->Init_For_LTA(name);
        m_templateMap.erase("Un-namedTemplate");
        m_templateMap[name] = tmplate;
        return Find_Template_Internal(name, true);
    } else {
        if (b && name.Is_Not_Empty()) {
            captainslog_dbgassert(0,
                "Failed to find thing template %s (case sensitive) This issue has a chance of crashing after you ignore it!",
                name.Str());
        }
        return nullptr;
    }
}

Object *ThingFactory::New_Object(const ThingTemplate *tmplate, Team *team, BitFlags<OBJECT_STATUS_COUNT> status_bits)
{
    // todo needs more of Object.
#ifdef GAME_DLL
    return Call_Method<Object *, ThingFactory, const ThingTemplate *, Team *, BitFlags<OBJECT_STATUS_COUNT>>(
        PICK_ADDRESS(0x004B0E10, 0x006C7B15), this, tmplate, team, status_bits);
#else
    return nullptr;
#endif
}

Drawable *ThingFactory::New_Drawable(const ThingTemplate *tmplate, DrawableStatus status_bits)
{
    if (tmplate == nullptr) {
        throw CODE_03;
    }

    return g_theGameClient->Create_Drawable(tmplate, status_bits);
}
