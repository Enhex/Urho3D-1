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

#pragma once


#include <Urho3D/Urho3DAll.h>
#include <cr/cr.h>
#include <Toolbox/SystemUI/AttributeInspector.h>
#include "Tabs/UI/UITab.h"
#include "IDPool.h"

using namespace std::placeholders;

namespace Urho3D
{

class Tab;
class SceneTab;
class AssetConverter;

class Editor : public Application
{
    URHO3D_OBJECT(Editor, Application);
public:
    /// Construct.
    explicit Editor(Context* context);
    /// Set up editor application.
    void Setup() override;
    /// Initialize editor application.
    void Start() override;
    /// Tear down editor application.
    void Stop() override;

    /// Save editor configuration.
    void SaveProject(String filePath);
    /// Load saved editor configuration.
    void LoadProject(String filePath);
    /// Renders UI elements.
    void OnUpdate(VariantMap& args);
    /// Renders menu bar at the top of the screen.
    void RenderMenuBar();
    /// Create a new tab of specified type.
    /// \param project is xml element containing serialized project data produced by SceneTab::SaveProject()
    template<typename T>
    T* CreateNewTab(XMLElement project=XMLElement());
    /// Return active scene tab.
    Tab* GetActiveTab() { return activeTab_; }
    /// Return currently open scene tabs.
    const Vector<SharedPtr<Tab>>& GetSceneViews() const { return tabs_; }
    /// Return a list of object categories registered with engine.
    StringVector GetObjectCategories() const;
    /// Return a map of names and type hashes from specified category.
    StringVector GetObjectsByCategory(const String& category);
    /// Get absolute path of `resourceName`. If it is empty, use `defaultResult`. If no resource is found then save file
    /// dialog will be invoked for selecting a new path.
    String GetResourceAbsolutePath(const String& resourceName, const String& defaultResult, const char* patterns,
    const String& dialogTitle);

protected:
    /// Process console commands.
    void OnConsoleCommand(VariantMap& args);
    /// Load a native user plugin from a specified shared library.
    bool LoadNativePlugin(const String& path);
    /// Returns true if specified path is internal engine or editor resource path.
    bool IsInternalResourcePath(const String& fullPath) const;

    struct NativePlugin
    {
        /// User plugin context.
        cr_plugin context_{};
        /// Path of user plugin.
        String path_;
    };

    /// Pool tracking availability of unique IDs used by editor.
    IDPool idPool_;
    /// List of active scene tabs.
    Vector<SharedPtr<Tab>> tabs_;
    /// Last focused scene tab.
    WeakPtr<Tab> activeTab_;
    /// Path to a project file.
    String projectFilePath_;
    /// Converter responsible for watching resource directories and converting assets to required formats.
    SharedPtr<AssetConverter> assetConverter_;
    Vector<String> engineResourcePaths_;
    Vector<String> engineResourcePrefixPaths_;
    Vector<String> engineResourceAutoloadPaths_;
    Vector<NativePlugin> nativePlugins_;
};

}
