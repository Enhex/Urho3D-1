//
// Copyright (c) 2018 Rokas Kupstys
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#include <Toolbox/SystemUI/ResourceBrowser.h>
#include <Toolbox/IO/ContentUtilities.h>
#include "Tabs/Scene/SceneTab.h"
#include "Tabs/UI/UITab.h"
#include "Editor.h"
#include "ResourceTab.h"

namespace Urho3D
{

static HashMap<ContentType, String> contentToTabType{
    {CTYPE_SCENE, "SceneTab"},
    {CTYPE_UILAYOUT, "UITab"},
};

unsigned MakeHash(ContentType value)
{
    return (unsigned)value;
}

ResourceTab::ResourceTab(Context* context)
    : Tab(context)
{
    isUtility_ = true;
    SetTitle("Resources");
}

bool ResourceTab::RenderWindowContent()
{
    if (ResourceBrowserWidget(resourcePath_, resourceSelection_) == RBR_ITEM_OPEN)
    {
        String selected = resourcePath_ + resourceSelection_;
        auto it = contentToTabType.Find(GetContentType(selected));
        if (it != contentToTabType.End())
        {
            auto* tab = GetSubsystem<Editor>()->CreateTab(it->second_);
            tab->AutoPlace();
            tab->LoadResource(selected);
        }
    }

    return true;
}

}
