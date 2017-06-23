﻿//
// MainPage.xaml.h
// Declaration of the MainPage class.
//

#pragma once

#include "MainPage.g.h"

namespace UnitTestsApp_WinRT_UWP
{
	/// <summary>
	/// An empty page that can be used on its own or navigated to within a Frame.
	/// </summary>
	public ref class MainPage sealed
	{
	public:
		MainPage();

        void OnClickRunButton(Platform::Object ^sender, Windows::UI::Xaml::RoutedEventArgs ^evArgs);
	};
}