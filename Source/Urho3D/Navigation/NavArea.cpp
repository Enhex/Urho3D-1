//
// Copyright (c) 2008-2015 the Urho3D project.
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

#include "../Core/Context.h"
#include "../Graphics/DebugRenderer.h"
#include "../Scene/Component.h"
#include "../Scene/Node.h"
#include "../Navigation/NavArea.h"

namespace Urho3D
{
    static const Vector3 DEFAULT_BOUNDING_BOX_MIN(-10.0f, -10.0f, -10.0f);
    static const Vector3 DEFAULT_BOUNDING_BOX_MAX(10.0f, 10.0f, 10.0f);
    static const unsigned DEFAULT_MASK_FLAG = 0;
    static const unsigned DEFAULT_AREA = 0;

    extern const char* NAVIGATION_CATEGORY;

    NavArea::NavArea(Context* context) :
        Component(context),
        areaType_(0),
        boundingBox_(DEFAULT_BOUNDING_BOX_MIN, DEFAULT_BOUNDING_BOX_MAX)
    {
    }

    NavArea::~NavArea()
    {
    }
    
    void NavArea::RegisterObject(Context* context)
    {
        context->RegisterFactory<NavArea>(NAVIGATION_CATEGORY);

        COPY_BASE_ATTRIBUTES(Component);
        ATTRIBUTE("Bounding Box Min", Vector3, boundingBox_.min_, DEFAULT_BOUNDING_BOX_MIN, AM_DEFAULT);
        ATTRIBUTE("Bounding Box Max", Vector3, boundingBox_.max_, DEFAULT_BOUNDING_BOX_MAX, AM_DEFAULT);
        ACCESSOR_ATTRIBUTE("Area Type", GetAreaType, SetAreaType, unsigned, DEFAULT_AREA, AM_DEFAULT);
    }

    void NavArea::SetAreaType(unsigned newType)
    {
        areaType_ = newType;
        MarkNetworkUpdate();
    }

    BoundingBox NavArea::GetWorldBoundingBox() const
    {
        Matrix3x4 mat;
        mat.SetTranslation(node_->GetWorldPosition());
        return boundingBox_.Transformed(mat);
    }

    void NavArea::DrawDebugGeometry(DebugRenderer* debug, bool depthTest) 
    {
        if (debug && IsEnabledEffective())
        {
            Matrix3x4 mat;
            mat.SetTranslation(node_->GetWorldPosition());
            debug->AddBoundingBox(boundingBox_, mat, Color::GREEN, depthTest);
        }
    }
}