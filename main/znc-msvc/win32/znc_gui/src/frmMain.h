#pragma once

#include "main.h"
#include "guiutils.hpp"
#include "zncrunner.hpp"

using namespace System;
using namespace System::ComponentModel;
using namespace System::Collections;
using namespace System::Windows::Forms;
using namespace System::Data;
using namespace System::Drawing;

static void LogOutputCallback(int type, const char* text, void *userData);

namespace znc_gui2 {

	/// <summary>
	/// Summary for frmMain
	///
	/// WARNING: If you change the name of this class, you will need to change the
	///          'Resource File Name' property for the managed resource compiler tool
	///          associated with all .resx files this class depends on.  Otherwise,
	///          the designers will not be able to interact properly with localized
	///          resources associated with this form.
	/// </summary>
	public ref class frmMain : public System::Windows::Forms::Form
	{
	public:
		static znc_gui2::frmMain^ m_instance;

		frmMain(void)
		{
			InitializeComponent();
			//
			//TODO: Add the constructor code here
			//
			m_instance = this;
		}

	protected:
		/// <summary>
		/// Clean up any resources being used.
		/// </summary>
		~frmMain()
		{
			if (components)
			{
				delete components;
			}
		}
	private: System::Windows::Forms::TabControl^  tabMain;
	protected: 

	private: System::Windows::Forms::TabPage^  tbpStatusControl;
	protected: 

	private: System::Windows::Forms::TabPage^  tbpLogConsole;



	private: System::Windows::Forms::StatusStrip^  stsStatus;
	private: System::Windows::Forms::Button^  btnStart;
	private: System::Windows::Forms::TextBox^  txtEventLog;



	private:
		/// <summary>
		/// Required designer variable.
		/// </summary>
		System::ComponentModel::Container ^components;

#pragma region Windows Form Designer generated code
		/// <summary>
		/// Required method for Designer support - do not modify
		/// the contents of this method with the code editor.
		/// </summary>
		void InitializeComponent(void)
		{
			this->tabMain = (gcnew System::Windows::Forms::TabControl());
			this->tbpStatusControl = (gcnew System::Windows::Forms::TabPage());
			this->btnStart = (gcnew System::Windows::Forms::Button());
			this->tbpLogConsole = (gcnew System::Windows::Forms::TabPage());
			this->txtEventLog = (gcnew System::Windows::Forms::TextBox());
			this->stsStatus = (gcnew System::Windows::Forms::StatusStrip());
			this->tabMain->SuspendLayout();
			this->tbpStatusControl->SuspendLayout();
			this->tbpLogConsole->SuspendLayout();
			this->SuspendLayout();
			// 
			// tabMain
			// 
			this->tabMain->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Bottom) 
				| System::Windows::Forms::AnchorStyles::Left) 
				| System::Windows::Forms::AnchorStyles::Right));
			this->tabMain->Controls->Add(this->tbpStatusControl);
			this->tabMain->Controls->Add(this->tbpLogConsole);
			this->tabMain->HotTrack = true;
			this->tabMain->Location = System::Drawing::Point(12, 12);
			this->tabMain->Name = L"tabMain";
			this->tabMain->SelectedIndex = 0;
			this->tabMain->Size = System::Drawing::Size(454, 309);
			this->tabMain->SizeMode = System::Windows::Forms::TabSizeMode::Fixed;
			this->tabMain->TabIndex = 0;
			// 
			// tbpStatusControl
			// 
			this->tbpStatusControl->Controls->Add(this->btnStart);
			this->tbpStatusControl->Location = System::Drawing::Point(4, 22);
			this->tbpStatusControl->Name = L"tbpStatusControl";
			this->tbpStatusControl->Padding = System::Windows::Forms::Padding(3);
			this->tbpStatusControl->Size = System::Drawing::Size(446, 283);
			this->tbpStatusControl->TabIndex = 0;
			this->tbpStatusControl->Text = L"Status Control";
			this->tbpStatusControl->UseVisualStyleBackColor = true;
			// 
			// btnStart
			// 
			this->btnStart->Location = System::Drawing::Point(33, 26);
			this->btnStart->Name = L"btnStart";
			this->btnStart->Size = System::Drawing::Size(125, 31);
			this->btnStart->TabIndex = 0;
			this->btnStart->Text = L"Start";
			this->btnStart->UseVisualStyleBackColor = true;
			this->btnStart->Click += gcnew System::EventHandler(this, &frmMain::btnStart_Click);
			// 
			// tbpLogConsole
			// 
			this->tbpLogConsole->Controls->Add(this->txtEventLog);
			this->tbpLogConsole->Location = System::Drawing::Point(4, 22);
			this->tbpLogConsole->Name = L"tbpLogConsole";
			this->tbpLogConsole->Padding = System::Windows::Forms::Padding(3);
			this->tbpLogConsole->Size = System::Drawing::Size(446, 283);
			this->tbpLogConsole->TabIndex = 1;
			this->tbpLogConsole->Text = L"Log Console";
			this->tbpLogConsole->UseVisualStyleBackColor = true;
			// 
			// txtEventLog
			// 
			this->txtEventLog->Location = System::Drawing::Point(3, 3);
			this->txtEventLog->Multiline = true;
			this->txtEventLog->Name = L"txtEventLog";
			this->txtEventLog->ScrollBars = System::Windows::Forms::ScrollBars::Both;
			this->txtEventLog->Size = System::Drawing::Size(440, 277);
			this->txtEventLog->TabIndex = 0;
			this->txtEventLog->WordWrap = false;
			// 
			// stsStatus
			// 
			this->stsStatus->Location = System::Drawing::Point(0, 333);
			this->stsStatus->Name = L"stsStatus";
			this->stsStatus->Size = System::Drawing::Size(478, 22);
			this->stsStatus->TabIndex = 1;
			this->stsStatus->Text = L"statusStrip1";
			// 
			// frmMain
			// 
			this->AutoScaleDimensions = System::Drawing::SizeF(6, 13);
			this->AutoScaleMode = System::Windows::Forms::AutoScaleMode::Font;
			this->ClientSize = System::Drawing::Size(478, 355);
			this->Controls->Add(this->stsStatus);
			this->Controls->Add(this->tabMain);
			this->FormBorderStyle = System::Windows::Forms::FormBorderStyle::FixedDialog;
			this->MaximizeBox = false;
			this->Name = L"frmMain";
			this->Text = L"ZNC";
			this->Load += gcnew System::EventHandler(this, &frmMain::frmMain_Load);
			this->tabMain->ResumeLayout(false);
			this->tbpStatusControl->ResumeLayout(false);
			this->tbpLogConsole->ResumeLayout(false);
			this->tbpLogConsole->PerformLayout();
			this->ResumeLayout(false);
			this->PerformLayout();

		}
#pragma endregion
	private: System::Void frmMain_Load(System::Object^  sender, System::EventArgs^  e) {
				 this->Icon = ZNCGUIUtils::GetIconFromRes(IDI_APPICON);
			 }
	private: System::Void btnStart_Click(System::Object^  sender, System::EventArgs^  e) {
				 if(!CZNCRunner::Get()->IsRunning())
				 {
					 CZNCRunner::Get()->SetOutputHook(&LogOutputCallback);

					 if(CZNCRunner::Get()->Start())
					 {
						 MessageBoxW(0, L"Started", L"ZNC", MB_ICONINFORMATION);
						 btnStart->Text = L"Stop";
					 }
				 }
				 else
				 {
					 if(CZNCRunner::Get()->Stop())
					 {
						 MessageBoxW(0, L"Stopped", L"ZNC", MB_ICONINFORMATION);
						 btnStart->Text = L"Start";
					 }
				 }
			 }

			 private:
				 String^ m_prevLogText;

			 delegate void DAddLogItemToList(System::String^);

			 System::Void AddLogItemToList(System::String^ a_text)
			 {
/*				 ListViewItem^ l_newItem = lvwLog->Items->Add(DateTime::Now.ToString("T") + " " + DateTime::Now.ToString("d"));
				 l_newItem->SubItems->Add(a_text);*/
				 txtEventLog->Text += a_text + "\r\n";
			 }

	 public: System::Void OnLogOutput(System::Int32 a_type, System::String^ a_text)
			  {
				/*String^ l_icon;

				switch(a_type)
				{
					case 2: l_icon = "error"; break;
					case 1: l_icon = "debug"; break;
					case 5:
					case 10: l_icon = "check"; break;
					case 11: l_icon = "error"; break;
					case -1: l_icon = "error"; break;
				}*/

				Invoke(gcnew DAddLogItemToList(this, &frmMain::AddLogItemToList), /*l_icon,*/ a_text);
			  }
};
}


static void LogOutputCallback(int type, const char* text, void *userData)
{
	znc_gui2::frmMain::m_instance->OnLogOutput(type, gcnew String(text));
}

