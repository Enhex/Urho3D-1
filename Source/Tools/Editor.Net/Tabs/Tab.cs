﻿//
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
using ImGui;
using Urho3D;


namespace Editor.Tabs
{
    public class Tab : Urho3D.Object
    {
        public struct Location
        {
            public string NextDockName;
            public DockSlot Slot;
        }

        private bool _isOpen = true;
        public bool IsOpen
        {
            get => _isOpen;
            set => _isOpen = value;
        }
        public string Title { get; protected set; }
        public System.Numerics.Vector2 InitialSize;
        public Location InitialLocation;
        protected WindowFlags _windowFlags = WindowFlags.Default;

        public Tab(Context context, string title, Vector2? initialSize = null, string placeNextToDock = null,
            DockSlot slot = DockSlot.SlotNone) : base(context)
        {
            Title = title;
            InitialSize = initialSize.GetValueOrDefault(new Vector2(-1, -1));
            InitialLocation.NextDockName = placeNextToDock;
            InitialLocation.Slot = slot;
        }

        public void OnRender()
        {
            ImGuiDock.SetNextDockPos(InitialLocation.NextDockName, InitialLocation.Slot, ImGui.Condition.FirstUseEver);
            if (ImGuiDock.BeginDock(Title, ref _isOpen, _windowFlags, InitialSize))
            {
                Render();
            }
            ImGuiDock.EndDock();
        }

        protected virtual void Render()
        {

        }
    }
}
