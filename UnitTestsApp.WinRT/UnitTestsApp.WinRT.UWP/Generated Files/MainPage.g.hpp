﻿//------------------------------------------------------------------------------
//     This code was generated by a tool.
//
//     Changes to this file may cause incorrect behavior and will be lost if
//     the code is regenerated.
//------------------------------------------------------------------------------
#include "pch.h"

#if defined _DEBUG && !defined DISABLE_XAML_GENERATED_BINDING_DEBUG_OUTPUT
extern "C" __declspec(dllimport) int __stdcall IsDebuggerPresent();
#endif

#include "MainPage.xaml.h"

void ::UnitTestsApp_WinRT_UWP::MainPage::InitializeComponent()
{
    if (_contentLoaded)
    {
        return;
    }
    _contentLoaded = true;
    ::Windows::Foundation::Uri^ resourceLocator = ref new ::Windows::Foundation::Uri(L"ms-appx:///MainPage.xaml");
    ::Windows::UI::Xaml::Application::LoadComponent(this, resourceLocator, ::Windows::UI::Xaml::Controls::Primitives::ComponentResourceLocation::Application);
}

void ::UnitTestsApp_WinRT_UWP::MainPage::Connect(int __connectionId, ::Platform::Object^ __target)
{
    switch (__connectionId)
    {
        case 1:
            {
                this->wideState = safe_cast<::Windows::UI::Xaml::VisualState^>(__target);
            }
            break;
        case 2:
            {
                this->narrowState = safe_cast<::Windows::UI::Xaml::VisualState^>(__target);
            }
            break;
        case 3:
            {
                this->LayoutRoot = safe_cast<::Windows::UI::Xaml::Controls::Grid^>(__target);
            }
            break;
        case 4:
            {
                this->titlePanel = safe_cast<::Windows::UI::Xaml::Controls::StackPanel^>(__target);
            }
            break;
        case 5:
            {
                this->ContentRoot = safe_cast<::Windows::UI::Xaml::Controls::Grid^>(__target);
            }
            break;
        case 6:
            {
                this->waitingRing = safe_cast<::Windows::UI::Xaml::Controls::ProgressRing^>(__target);
            }
            break;
        case 7:
            {
                this->runButton = safe_cast<::Windows::UI::Xaml::Controls::Button^>(__target);
                (safe_cast<::Windows::UI::Xaml::Controls::Button^>(this->runButton))->Click += ref new ::Windows::UI::Xaml::RoutedEventHandler(this, (void (::UnitTestsApp_WinRT_UWP::MainPage::*)
                    (::Platform::Object^, ::Windows::UI::Xaml::RoutedEventArgs^))&MainPage::OnClickRunButton);
            }
            break;
        case 8:
            {
                this->mainTextBlock = safe_cast<::Windows::UI::Xaml::Controls::TextBlock^>(__target);
            }
            break;
        case 9:
            {
                this->titleTxtBlock1 = safe_cast<::Windows::UI::Xaml::Controls::TextBlock^>(__target);
            }
            break;
        case 10:
            {
                this->titleTxtBlock2 = safe_cast<::Windows::UI::Xaml::Controls::TextBlock^>(__target);
            }
            break;
    }
    _contentLoaded = true;
}

::Windows::UI::Xaml::Markup::IComponentConnector^ ::UnitTestsApp_WinRT_UWP::MainPage::GetBindingConnector(int __connectionId, ::Platform::Object^ __target)
{
    __connectionId;         // unreferenced
    __target;               // unreferenced
    return nullptr;
}


