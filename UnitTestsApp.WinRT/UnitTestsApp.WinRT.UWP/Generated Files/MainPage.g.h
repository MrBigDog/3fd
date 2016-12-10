﻿#pragma once
//------------------------------------------------------------------------------
//     This code was generated by a tool.
//
//     Changes to this file may cause incorrect behavior and will be lost if
//     the code is regenerated.
//------------------------------------------------------------------------------


namespace Windows {
    namespace UI {
        namespace Xaml {
            ref class VisualState;
        }
    }
}
namespace Windows {
    namespace UI {
        namespace Xaml {
            namespace Controls {
                ref class Grid;
                ref class StackPanel;
                ref class ProgressRing;
                ref class Button;
                ref class TextBlock;
            }
        }
    }
}

namespace UnitTestsApp_WinRT_UWP
{
    [::Windows::Foundation::Metadata::WebHostHidden]
    partial ref class MainPage : public ::Windows::UI::Xaml::Controls::Page, 
        public ::Windows::UI::Xaml::Markup::IComponentConnector,
        public ::Windows::UI::Xaml::Markup::IComponentConnector2
    {
    public:
        void InitializeComponent();
        virtual void Connect(int connectionId, ::Platform::Object^ target);
        virtual ::Windows::UI::Xaml::Markup::IComponentConnector^ GetBindingConnector(int connectionId, ::Platform::Object^ target);
    
    private:
        bool _contentLoaded;
    
        private: ::Windows::UI::Xaml::VisualState^ wideState;
        private: ::Windows::UI::Xaml::VisualState^ narrowState;
        private: ::Windows::UI::Xaml::Controls::Grid^ LayoutRoot;
        private: ::Windows::UI::Xaml::Controls::StackPanel^ titlePanel;
        private: ::Windows::UI::Xaml::Controls::Grid^ ContentRoot;
        private: ::Windows::UI::Xaml::Controls::ProgressRing^ waitingRing;
        private: ::Windows::UI::Xaml::Controls::Button^ runButton;
        private: ::Windows::UI::Xaml::Controls::TextBlock^ mainTextBlock;
        private: ::Windows::UI::Xaml::Controls::TextBlock^ titleTxtBlock1;
        private: ::Windows::UI::Xaml::Controls::TextBlock^ titleTxtBlock2;
    };
}

