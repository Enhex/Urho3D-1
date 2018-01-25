//
// Copyright (c) 2008-2017 the Urho3D project.
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

#include <IconFontCppHeaders/IconsFontAwesome.h>

#include <Toolbox/Scene/DebugCameraController.h>
#include <Toolbox/SystemUI/Widgets.h>
#include <ImGuizmo/ImGuizmo.h>

#include "SceneTab.h"
#include "EditorEvents.h"
#include "Editor.h"
#include "Widgets.h"
#include "SceneSettings.h"
#include <ImGui/imgui_internal.h>


namespace Urho3D
{

SceneTab::SceneTab(Context* context, StringHash id, const String& afterDockName, ui::DockSlot_ position)
    : Tab(context, id, afterDockName, position)
    , view_(context, {0, 0, 1024, 768})
    , gizmo_(context)
    , undo_(context)
    , sceneState_(context)
{
    SetTitle("New Scene");
    windowFlags_ = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;

    settings_ = new SceneSettings(context);
    effectSettings_ = new SceneEffects(this);

    SubscribeToEvent(this, E_EDITORSELECTIONCHANGED, std::bind(&SceneTab::OnNodeSelectionChanged, this));
    SubscribeToEvent(effectSettings_, E_EDITORSCENEEFFECTSCHANGED, std::bind(&AttributeInspector::CopyEffectsFrom,
        &inspector_, view_.GetViewport()));
    SubscribeToEvent(E_UPDATE, std::bind(&SceneTab::OnUpdate, this, _2));

    undo_.Connect(GetScene());
    undo_.Connect(&inspector_);
    undo_.Connect(&gizmo_);

    SubscribeToEvent(GetScene(), E_ASYNCLOADFINISHED, [&](StringHash, VariantMap&) {
        undo_.Clear();
    });

    // Scene is updated manually.
    GetScene()->SetUpdateEnabled(false);

    CreateObjects();
    undo_.Clear();
}

SceneTab::~SceneTab() = default;

bool SceneTab::RenderWindowContent()
{
    auto& style = ui::GetStyle();
    if (GetInput()->IsMouseVisible())
        lastMousePosition_ = GetInput()->GetMousePosition();
    bool open = true;

    // Focus window when appearing
    if (!isRendered_)
    {
        ui::SetWindowFocus();
        effectSettings_->Prepare(true);
    }
    ImGuizmo::SetDrawlist();

    RenderToolbarButtons();
    IntRect tabRect = UpdateViewRect();

    ui::SetCursorScreenPos(ToImGui(tabRect.Min()));
    ui::Image(view_.GetTexture(), ToImGui(tabRect.Size()));

    bool isClickedLeft = ui::IsMouseReleased(MOUSEB_LEFT) && ui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup);
    bool isClickedRight = ui::IsMouseReleased(MOUSEB_RIGHT) && ui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup);

    view_.GetCamera()->GetNode()->GetComponent<DebugCameraController>()->SetEnabled(isActive_);
    gizmo_.ManipulateSelection(view_.GetCamera());

    // Prevent dragging window when scene view is clicked.
    if (ui::IsWindowHovered())
        windowFlags_ |= ImGuiWindowFlags_NoMove;
    else
        windowFlags_ &= ~ImGuiWindowFlags_NoMove;

    if (!gizmo_.IsActive() && (isClickedLeft || isClickedRight) && GetInput()->IsMouseVisible())
    {
        // Handle object selection.
        IntVector2 pos = GetInput()->GetMousePosition();
        pos -= tabRect.Min();

        Ray cameraRay = view_.GetCamera()->GetScreenRay((float)pos.x_ / tabRect.Width(), (float)pos.y_ / tabRect.Height());
        // Pick only geometry objects, not eg. zones or lights, only get the first (closest) hit
        PODVector<RayQueryResult> results;

        RayOctreeQuery query(results, cameraRay, RAY_TRIANGLE, M_INFINITY, DRAWABLE_GEOMETRY);
        GetScene()->GetComponent<Octree>()->RaycastSingle(query);

        if (!results.Size())
        {
            // When object geometry was not hit by a ray - query for object bounding box.
            RayOctreeQuery query2(results, cameraRay, RAY_OBB, M_INFINITY, DRAWABLE_GEOMETRY);
            GetScene()->GetComponent<Octree>()->RaycastSingle(query2);
        }

        if (results.Size())
        {
            WeakPtr<Node> clickNode(results[0].drawable_->GetNode());
            bool appendSelection = GetInput()->GetKeyDown(KEY_CTRL);
            if (!appendSelection)
                UnselectAll();
            ToggleSelection(clickNode);

            if (isClickedRight)
                ui::OpenPopupEx(ui::GetID("Node context menu"), true);
        }
        else
            UnselectAll();
    }

    RenderNodeContextMenu();

    const auto tabContextMenuTitle = "SceneTab context menu";
    if (ui::IsDockTabHovered() && GetInput()->GetMouseButtonPress(MOUSEB_RIGHT))
        ui::OpenPopup(tabContextMenuTitle);
    if (ui::BeginPopup(tabContextMenuTitle))
    {
        if (ui::MenuItem("Save"))
            Tab::SaveResource();

        ui::Separator();

        if (ui::MenuItem("Close"))
            open = false;

        ui::EndPopup();
    }

    return open;
}

void SceneTab::LoadResource(const String& resourcePath)
{
    if (resourcePath.Empty())
        return;

    if (resourcePath.EndsWith(".xml", false))
    {
        XMLFile* file = GetCache()->GetResource<XMLFile>(resourcePath);
        if (file && GetScene()->LoadXML(file->GetRoot()))
        {
            path_ = resourcePath;
            CreateObjects();
        }
        else
            URHO3D_LOGERRORF("Loading scene %s failed", GetFileName(resourcePath).CString());
    }
    else if (resourcePath.EndsWith(".json", false))
    {
        JSONFile* file = GetCache()->GetResource<JSONFile>(resourcePath);
        if (file && GetScene()->LoadJSON(file->GetRoot()))
        {
            path_ = resourcePath;
            CreateObjects();
        }
        else
            URHO3D_LOGERRORF("Loading scene %s failed", GetFileName(resourcePath).CString());
    }
    else
        URHO3D_LOGERRORF("Unknown scene file format %s", GetExtension(resourcePath).CString());

    SetTitle(GetFileName(path_));
}

bool SceneTab::SaveResource(const String& resourcePath)
{
    auto fullPath = GetSubsystem<Editor>()->GetResourceAbsolutePath(resourcePath, path_, "xml", "Save Scene As");
    if (fullPath.Empty())
        return false;

    File file(context_, fullPath, FILE_WRITE);
    bool result = false;

    float elapsed = 0;
    if (!settings_->saveElapsedTime_)
    {
        elapsed = GetScene()->GetElapsedTime();
        GetScene()->SetElapsedTime(0);
    }

    GetScene()->SetUpdateEnabled(true);
    if (fullPath.EndsWith(".xml", false))
        result = GetScene()->SaveXML(file);
    else if (fullPath.EndsWith(".json", false))
        result = GetScene()->SaveJSON(file);
    GetScene()->SetUpdateEnabled(false);

    if (!settings_->saveElapsedTime_)
        GetScene()->SetElapsedTime(elapsed);

    if (result)
    {
        if (!resourcePath.Empty())
        {
            path_ = resourcePath;
            SetTitle(GetFileName(path_));
        }
    }
    else
        URHO3D_LOGERRORF("Saving scene to %s failed.", resourcePath.CString());

    if (result)
        SendEvent(E_EDITORRESOURCESAVED);

    return result;
}

void SceneTab::CreateObjects()
{
    view_.CreateObjects();
    view_.GetCamera()->GetNode()->CreateComponent<DebugCameraController>();
}

void SceneTab::Select(Node* node)
{
    if (gizmo_.Select(node))
    {
        using namespace EditorSelectionChanged;
        SendEvent(E_EDITORSELECTIONCHANGED, P_SCENETAB, this);
    }
}

void SceneTab::Unselect(Node* node)
{
    if (gizmo_.Unselect(node))
    {
        using namespace EditorSelectionChanged;
        SendEvent(E_EDITORSELECTIONCHANGED, P_SCENETAB, this);
    }
}

void SceneTab::ToggleSelection(Node* node)
{
    gizmo_.ToggleSelection(node);
    using namespace EditorSelectionChanged;
    SendEvent(E_EDITORSELECTIONCHANGED, P_SCENETAB, this);
}

void SceneTab::UnselectAll()
{
    if (gizmo_.UnselectAll())
    {
        using namespace EditorSelectionChanged;
        SendEvent(E_EDITORSELECTIONCHANGED, P_SCENETAB, this);
    }
}

const Vector<WeakPtr<Node>>& SceneTab::GetSelection() const
{
    return gizmo_.GetSelection();
}

void SceneTab::RenderToolbarButtons()
{
    auto& style = ui::GetStyle();
    auto oldRounding = style.FrameRounding;
    style.FrameRounding = 0;

    if (ui::EditorToolbarButton(ICON_FA_FLOPPY_O, "Save"))
        Tab::SaveResource();

    ui::SameLine(0, 3.f);

    if (ui::EditorToolbarButton(ICON_FA_UNDO, "Undo"))
        undo_.Undo();
    if (ui::EditorToolbarButton(ICON_FA_REPEAT, "Redo"))
        undo_.Redo();

    ui::SameLine(0, 3.f);

    if (ui::EditorToolbarButton(ICON_FA_ARROWS, "Translate", gizmo_.GetOperation() == GIZMOOP_TRANSLATE))
        gizmo_.SetOperation(GIZMOOP_TRANSLATE);
    if (ui::EditorToolbarButton(ICON_FA_REPEAT, "Rotate", gizmo_.GetOperation() == GIZMOOP_ROTATE))
        gizmo_.SetOperation(GIZMOOP_ROTATE);
    if (ui::EditorToolbarButton(ICON_FA_ARROWS_ALT, "Scale", gizmo_.GetOperation() == GIZMOOP_SCALE))
        gizmo_.SetOperation(GIZMOOP_SCALE);

    ui::SameLine(0, 3.f);

    if (ui::EditorToolbarButton(ICON_FA_ARROWS_ALT, "World", gizmo_.GetTransformSpace() == TS_WORLD))
        gizmo_.SetTransformSpace(TS_WORLD);
    if (ui::EditorToolbarButton(ICON_FA_ARROWS_ALT, "Local", gizmo_.GetTransformSpace() == TS_LOCAL))
        gizmo_.SetTransformSpace(TS_LOCAL);

    ui::SameLine(0, 3.f);

    if (Light* light = view_.GetCamera()->GetNode()->GetComponent<Light>())
    {
        if (ui::EditorToolbarButton(ICON_FA_LIGHTBULB_O, "Camera Headlight", light->IsEnabled()))
            light->SetEnabled(!light->IsEnabled());
    }

    ui::SameLine(0, 3.f);

    if (ui::EditorToolbarButton(scenePlaying_ ? ICON_FA_PAUSE : ICON_FA_PLAY, scenePlaying_ ? "Pause" : "Play"))
    {
        Scene* scene = view_.GetScene();
        scenePlaying_ ^= true;
        if (scenePlaying_)
        {
            undo_.SetTrackingEnabled(false);
            XMLElement root = sceneState_.CreateRoot("scene");
            scene->SaveXML(root);
        }
        else
        {
            // Migrade editor objects to a newly loaded scene without destroying them.
            Vector<SharedPtr<Node>> temporaries;
            for (auto& node : scene->GetChildren())
            {
                if (node->IsTemporary())
                    temporaries.Push(SharedPtr<Node>(node));
            }
            scene->LoadXML(sceneState_.GetRoot());
            for (auto& node : temporaries)
                scene->AddChild(node);
            undo_.SetTrackingEnabled(true);
            sceneState_.GetRoot().Remove();
        }
    }

    ui::NewLine();
    style.FrameRounding = oldRounding;
}

bool SceneTab::IsSelected(Node* node) const
{
    return gizmo_.IsSelected(node);
}

void SceneTab::OnNodeSelectionChanged()
{
    using namespace EditorSelectionChanged;
    selectedComponent_ = nullptr;
}

void SceneTab::RenderInspector()
{
    // TODO: inspector for multi-selection.
    if (GetSelection().Size() == 1)
    {
        auto node = GetSelection().Front();
        if (node.Expired())
            return;

        PODVector<Serializable*> items;
        items.Push(node.Get());
        if (node == GetScene())
        {
            effectSettings_->Prepare();
            items.Push(settings_.Get());
            items.Push(effectSettings_.Get());
        }
        for (Component* component : node->GetComponents())
            items.Push(component);
        inspector_.RenderAttributes(items);
    }
}

void SceneTab::RenderNodeTree()
{
    auto oldSpacing = ui::GetStyle().IndentSpacing;
    ui::GetStyle().IndentSpacing = 10;
    RenderNodeTree(GetScene());
    ui::GetStyle().IndentSpacing = oldSpacing;
}

void SceneTab::RenderNodeTree(Node* node)
{
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow;
    if (node->GetParent() == nullptr)
        flags |= ImGuiTreeNodeFlags_DefaultOpen;

    if (node->IsTemporary())
        return;

    String name = ToString("%s (%d)", (node->GetName().Empty() ? node->GetTypeName() : node->GetName()).CString(), node->GetID());
    bool isSelected = IsSelected(node) && selectedComponent_.Expired();

    if (isSelected)
        flags |= ImGuiTreeNodeFlags_Selected;

    ui::Image("Node");
    ui::SameLine();
    auto opened = ui::TreeNodeEx(name.CString(), flags);
    if (!opened)
    {
        // If TreeNode above is opened, it pushes it's label as an ID to the stack. However if it is not open then no
        // ID is pushed. This creates a situation where context menu is not properly attached to said tree node due to
        // missing ID on the stack. To correct this we ensure that ID is always pushed. This allows us to show context
        // menus even for closed tree nodes.
        ui::PushID(name.CString());
    }

    // Popup may delete node. Weak reference will convey that information.
    WeakPtr<Node> nodeRef(node);

    if (ui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup))
    {
        if (ui::IsMouseClicked(MOUSEB_LEFT))
        {
            if (!GetInput()->GetKeyDown(KEY_CTRL))
                UnselectAll();
            ToggleSelection(node);
        }
        else if (ui::IsMouseClicked(MOUSEB_RIGHT) && !scenePlaying_)
        {
            UnselectAll();
            ToggleSelection(node);
            ui::OpenPopupEx(ui::GetID("Node context menu"), true);
        }
    }

    RenderNodeContextMenu();

    if (opened)
    {
        if (!nodeRef.Expired())
        {
            for (auto& component: node->GetComponents())
            {
                if (component->IsTemporary())
                    continue;

                ui::PushID(component);

                ui::Image(component->GetTypeName());
                ui::SameLine();

                bool selected = selectedComponent_ == component;
                selected = ui::Selectable(component->GetTypeName().CString(), selected);

                if (ui::IsMouseClicked(MOUSEB_RIGHT) && ui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup))
                {
                    selected = true;
                    ui::OpenPopupEx(ui::GetID("Component context menu"), true);
                }

                if (selected)
                {
                    UnselectAll();
                    ToggleSelection(node);
                    selectedComponent_ = component;
                }

                if (ui::BeginPopup("Component context menu"))
                {
                    if (ui::MenuItem("Remove"))
                        selectedComponent_->Remove();
                    ui::EndPopup();
                }

                ui::PopID();
            }

            // Do not use element->GetChildren() because child may be deleted during this loop.
            PODVector<Node*> children;
            node->GetChildren(children);
            for (Node* child: children)
                RenderNodeTree(child);
        }
        ui::TreePop();
    }
    else
        ui::PopID();
}

void SceneTab::LoadProject(XMLElement& scene)
{
    id_ = StringHash(ToUInt(scene.GetAttribute("id"), 16));
    LoadResource(scene.GetAttribute("path"));

    auto camera = scene.GetChild("camera");
    if (camera.NotNull())
    {
        Node* cameraNode = view_.GetCamera()->GetNode();
        if (auto position = camera.GetChild("position"))
            cameraNode->SetPosition(position.GetVariant().GetVector3());
        if (auto rotation = camera.GetChild("rotation"))
            cameraNode->SetRotation(rotation.GetVariant().GetQuaternion());
        if (auto light = camera.GetChild("light"))
            cameraNode->GetComponent<Light>()->SetEnabled(light.GetVariant().GetBool());
    }

    settings_->LoadProject(scene);
    effectSettings_->LoadProject(scene);

    undo_.Clear();
}

void SceneTab::SaveProject(XMLElement& scene)
{
    scene.SetAttribute("type", "scene");
    scene.SetAttribute("id", id_.ToString().CString());
    scene.SetAttribute("path", path_);

    auto camera = scene.CreateChild("camera");
    Node* cameraNode = view_.GetCamera()->GetNode();
    camera.CreateChild("position").SetVariant(cameraNode->GetPosition());
    camera.CreateChild("rotation").SetVariant(cameraNode->GetRotation());
    camera.CreateChild("light").SetVariant(cameraNode->GetComponent<Light>()->IsEnabled());

    settings_->SaveProject(scene);
    effectSettings_->SaveProject(scene);
    Tab::SaveResource();
}

void SceneTab::ClearCachedPaths()
{
    path_.Clear();
}

void SceneTab::OnActiveUpdate()
{
    if (ui::IsAnyItemActive() || scenePlaying_)
        return;

    Input* input = GetSubsystem<Input>();

    if (input->GetKeyDown(KEY_CTRL))
    {
        if (input->GetKeyPress(KEY_Y) || (input->GetKeyDown(KEY_SHIFT) && input->GetKeyPress(KEY_Z)))
            undo_.Redo();
        else if (input->GetKeyPress(KEY_Z))
            undo_.Undo();
    }

    if (input->GetKeyPress(KEY_DELETE))
        RemoveSelection();
}

void SceneTab::RemoveSelection()
{
    if (!selectedComponent_.Expired())
        selectedComponent_->Remove();

    for (auto& selected : GetSelection())
    {
        if (!selected.Expired())
            selected->Remove();
    }
    UnselectAll();
}

IntRect SceneTab::UpdateViewRect()
{
    IntRect tabRect = Tab::UpdateViewRect();
    view_.SetSize(tabRect);
    gizmo_.SetScreenRect(tabRect);
    return tabRect;
}

void SceneTab::OnUpdate(VariantMap& args)
{
    float timeStep = args[Update::P_TIMESTEP].GetFloat();

    if (scenePlaying_)
        GetScene()->Update(timeStep);

    view_.GetCamera()->GetComponent<DebugCameraController>()->Update(timeStep);
}

void SceneTab::RenderNodeContextMenu()
{
    if (ui::BeginPopup("Node context menu") && !scenePlaying_)
    {
        Input* input = GetSubsystem<Input>();
        if (input->GetKeyPress(KEY_ESCAPE) || !input->IsMouseVisible())
        {
            // Close when interacting with scene camera.
            ui::CloseCurrentPopup();
            ui::EndPopup();
            return;
        }

        bool alternative = input->GetKeyDown(KEY_SHIFT);

        if (ui::MenuItem(alternative ? "Create Child (Local)" : "Create Child"))
        {
            for (auto& selectedNode : GetSelection())
            {
                if (!selectedNode.Expired())
                    Select(selectedNode->CreateChild(String::EMPTY, alternative ? LOCAL : REPLICATED));
            }
        }

        if (ui::BeginMenu(alternative ? "Create Component (Local)" : "Create Component"))
        {
            Editor* editor = GetSubsystem<Editor>();
            auto categories = editor->GetObjectCategories();
            categories.Remove("UI");

            for (const String& category : categories)
            {
                if (ui::BeginMenu(category.CString()))
                {
                    auto components = editor->GetObjectsByCategory(category);
                    Sort(components.Begin(), components.End());

                    for (const String& component : components)
                    {
                        ui::Image(component);
                        ui::SameLine();
                        if (ui::MenuItem(component.CString()))
                        {
                            for (auto& selectedNode : GetSelection())
                            {
                                if (!selectedNode.Expired())
                                    selectedNode->CreateComponent(StringHash(component),
                                        alternative ? LOCAL : REPLICATED);
                            }
                        }
                    }
                    ui::EndMenu();
                }
            }
            ui::EndMenu();
        }

        ui::Separator();

        if (ui::MenuItem("Remove"))
            RemoveSelection();

        ui::EndPopup();
    }
}

}
