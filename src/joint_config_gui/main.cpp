// Windows API�֘A
#include <Windows.h>
#include <CommCtrl.h>
#pragma comment(lib, "ComCtl32.lib")

// �W��C++���C�u����
#include <iomanip>
#include <sstream>
#include <cstdint>

// �Ǝ��������C�u����
#include "resource.h"
#include "bgapi/cmd_def.h"


namespace BGAPI
{
	volatile HANDLE bled112_handle;
	volatile bool   handle_created = false;
	volatile bool   cmd_success    = false;
	volatile bool   connected      = false;

	// BLED112�ւ̃R�}���h���M
	// ========================================================================
	// NOTE:
	// "cmd_def.c"����"bglib_output"�|�C���^�Ɋ֐��|�C���^�������邱�ƂŁA
	// �R�}���h���M�������Ϗ����Ă��炢�܂��B(BGAPI�����ˑ��ɂȂ�Ȃ��H�v)
	void output(std::uint8_t header_len, std::uint8_t* header, std::uint16_t msg_len, std::uint8_t* msg)
	{
		DWORD written_size;

		if (WriteFile(BGAPI::bled112_handle, header, header_len, &written_size, NULL) == 0)
		{
			MessageBox(NULL, "BLED112�ւ̃R�}���h���M�Ɏ��s���܂����B", "Error!", MB_OK);

			return;
		}

		if (WriteFile(BGAPI::bled112_handle, msg, msg_len, &written_size, NULL) == 0)
		{
			MessageBox(NULL, "BLED112�ւ̃R�}���h���M�Ɏ��s���܂����B", "Error!", MB_OK);

			return;
		}
	}

	// BLED112����̃��X�|���X������
	// ========================================================================
	// NOTE:
	// ���̊֐���BGAPI���Ăяo������A�K�v�ȉ񐔂����Ăяo���K�v������܂��B
	// �ǂ̃R�}���h�ł����X�|���X���K��1�񔭐�����̂ŁA�Œ�1��͕K�v�ł��B
	// �s����ɃC�x���g�Ƃ��ă��X�|���X���Ԃ��Ă���ꍇ�́A���[�v������
	// �s���K�v������܂��B(�{�A�v���ł��A)
	//
	// CAUTION:
	// ���ݔ񓯊��ǂݍ��݂��s���Ă��Ȃ����߁A�ǂݍ��ݕ����Ńr�W�[�E�F�C�g��
	// ������܂��B�ʃX���b�h�ɓǂݍ��ݕ�����C����������������̂ł����A
	// �s�v�c�ȋ��������܂����B(�����炭�ǂݏ����̔r�����������K�v������܂��B)
	int readMessage()
	{
		DWORD read_size;
		struct ble_header header;

		BOOL ret = ReadFile(BGAPI::bled112_handle, reinterpret_cast<unsigned char*>(&header), 4, &read_size, NULL);
		if(!ret)
		{
			return GetLastError();
		}

		if (read_size == 0)
		{
			return 0;
		}

		unsigned char data_buff[256];
		
		ret = ReadFile(BGAPI::bled112_handle, data_buff, header.lolen, &read_size, NULL);
		if(header.lolen)
		{
			if(!ret)
			{
				return GetLastError();
			}
		}

		const struct ble_msg* msg = ble_get_msg_hdr(header);
		if(!msg)
		{
			MessageBox(NULL, "�Ή����郁�b�Z�[�W�n���h�������݂��܂���B", "Error!", MB_OK);

			return -1;
		}

		// ���b�Z�[�W�n���h���ɏ������Ϗ� (�e�C�x���g�n���h���̎����́Able_handler.cpp�����Q��)
		msg->handler(data_buff);

		return 0;
	}
}

namespace GUI
{
	volatile HWND main_dlg;
	volatile int  checked_joint_id = 0;
}


namespace Joint
{
	struct Settings
	{
		unsigned int min;
		unsigned int max;
		unsigned int home;
		unsigned int now;
	};

	int map[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 12, 13, 14, 15, 16, 17, 18, 19, 20 };
	Settings settings[] =
	{
		{ 250, 1550, 900,  900  },
		{ 250, 1550, 1150, 1150 },
		{ 250, 1550, 1200, 1200 },
		{ 250, 1550, 800,  800  },
		{ 250, 1550, 800,  800  },
		{ 250, 1550, 850,  850  },
		{ 250, 1550, 1400, 1400 },
		{ 250, 1550, 1200, 1200 },
		{ 250, 1550, 850,  850  },
		{ 250, 1550, 900,  900  },
		{ 250, 1550, 950,  950  },
		{ 250, 1550, 600,  600  },
		{ 250, 1550, 1100, 1100 },
		{ 250, 1550, 1000, 1000 },
		{ 250, 1550, 1100, 1100 },
		{ 250, 1550, 400,  400  },
		{ 250, 1550, 580,  580  },
		{ 250, 1550, 1000, 1000 }
	};
}


namespace
{
	void loadComList(HWND hWnd)
	{
		HKEY hkey;

		LONG ret = RegOpenKeyEx(HKEY_LOCAL_MACHINE, "HARDWARE\\DEVICEMAP\\SERIALCOMM", 0, KEY_READ, &hkey);
		if (ret != ERROR_SUCCESS)
		{
			return;
		}

		DWORD key_count;
		
		ret = RegQueryInfoKey(hkey, NULL, NULL, NULL, NULL, NULL, NULL, &key_count, NULL, NULL, NULL, NULL);
		if (ret != ERROR_SUCCESS)
		{
			return;
		}

		for (int index = 0; index < key_count; index++)
		{
			char  name_buff[256];
			BYTE  data_buff[256];
			DWORD name_buff_size = 256;
			DWORD data_buff_size = 256;
			DWORD type = 0;

			ret = RegEnumValue(hkey, index, name_buff, &name_buff_size, NULL, &type, data_buff, &data_buff_size);
			if(ret != ERROR_SUCCESS )
			{
				return;
			}

			SendDlgItemMessage(hWnd, LIST_COM, LB_ADDSTRING, 0, (LPARAM)data_buff);
		}
	}

	void setJointSettingMax(HWND hWnd)
	{
		std::stringstream max_button_caption;
		max_button_caption << "MAX (" << Joint::settings[GUI::checked_joint_id].max << ")";
		SetDlgItemText(hWnd, BUTTON_MAX, max_button_caption.str().c_str());
	}

	void setJointSettingMin(HWND hWnd)
	{
		std::stringstream min_button_caption;
		min_button_caption << "MIN (" << Joint::settings[GUI::checked_joint_id].min << ")";
		SetDlgItemText(hWnd, BUTTON_MIN, min_button_caption.str().c_str());
	}

	void setJointSettingHome(HWND hWnd)
	{
		std::stringstream home_button_caption;
		home_button_caption << "HOME (" << Joint::settings[GUI::checked_joint_id].home << ")";
		SetDlgItemText(hWnd, BUTTON_HOME, home_button_caption.str().c_str());
	}

	void setJointSettingNow(HWND hWnd, bool init = false)
	{
		std::stringstream position_str;
		
		if (init)
		{
			position_str << Joint::settings[GUI::checked_joint_id].home;
			SendDlgItemMessage(hWnd, SLIDER_ANGLE, TBM_SETPOS, TRUE, 1800 - Joint::settings[GUI::checked_joint_id].home);
		}
		else
		{
			position_str << Joint::settings[GUI::checked_joint_id].now;
			SendDlgItemMessage(hWnd, SLIDER_ANGLE, TBM_SETPOS, TRUE, 1800 - Joint::settings[GUI::checked_joint_id].now);
		}

		SetDlgItemText(hWnd, STEXT_ANGLE, position_str.str().c_str());
	}

	void loadJointSetting(HWND hWnd, bool init = false)
	{
		setJointSettingMin(hWnd);
		setJointSettingMax(hWnd);
		setJointSettingHome(hWnd);
		setJointSettingNow(hWnd, init);
	}

	std::string buildCmd(HWND hWnd)
	{
		int angle = 1800 - SendDlgItemMessage(hWnd, SLIDER_ANGLE, TBM_GETPOS, 0, 0);
		
		std::stringstream cmd;
		cmd << std::setfill('0') << std::setw(2) << std::hex << static_cast<std::int16_t>(Joint::map[GUI::checked_joint_id]);
		cmd << std::setfill('0') << std::setw(3) << std::hex << static_cast<std::int16_t>(angle);

		return cmd.str();
	}
}


BOOL CALLBACK mainDlgProc(HWND hDlg, UINT msg, WPARAM wp, LPARAM lp)
{
	switch (msg)
	{
		case WM_INITDIALOG:
		{
			InitCommonControls();
			SendDlgItemMessage(hDlg, RADIO_JOINT01, BM_SETCHECK, TRUE, BST_CHECKED);
			SendDlgItemMessage(hDlg, SLIDER_ANGLE, TBM_SETRANGEMAX, TRUE, 1800);
			SendDlgItemMessage(hDlg, SLIDER_ANGLE, TBM_SETRANGEMIN, TRUE, 0);
			SendDlgItemMessage(hDlg, SLIDER_ANGLE, TBM_SETPAGESIZE, 0, 25);
			::loadJointSetting(hDlg);
			::loadComList(hDlg);

			::bglib_output = BGAPI::output;
			GUI::main_dlg  = hDlg;

			return TRUE;
		}

		case WM_VSCROLL:
		{
			if ((HWND)lp == GetDlgItem(hDlg, SLIDER_ANGLE))
			{
				int position = SendDlgItemMessage(hDlg, SLIDER_ANGLE, TBM_GETPOS, 0, 0);

				Joint::settings[GUI::checked_joint_id].now = 1800 - position;
				::setJointSettingNow(hDlg);

				if (BGAPI::connected)
				{
					std::string cmd = "#SA" + ::buildCmd(hDlg);
					ble_cmd_attclient_attribute_write(0, 31, 8, cmd.c_str());
					Sleep(10);

					BGAPI::readMessage();
					BGAPI::readMessage();
				}
			}

			return TRUE;
		}

		case WM_COMMAND:
		{
			switch (LOWORD(wp))
			{
				case BUTTON_DOWN:
				{
					int position = SendDlgItemMessage(hDlg, SLIDER_ANGLE, TBM_GETPOS, 0, 0);

					if (position < 1800)
					{
						SendDlgItemMessage(hDlg, SLIDER_ANGLE, TBM_SETPOS, TRUE, ++position);

						Joint::settings[GUI::checked_joint_id].now = 1800 - position;
						::setJointSettingNow(hDlg);

						if (BGAPI::connected)
						{
							std::string cmd = "#SA" + ::buildCmd(hDlg);
							ble_cmd_attclient_attribute_write(0, 31, 8, cmd.c_str());
							Sleep(10);

							BGAPI::readMessage();
							BGAPI::readMessage();
						}
					}

					break;
				}

				case BUTTON_UP:
				{
					int position = SendDlgItemMessage(hDlg, SLIDER_ANGLE, TBM_GETPOS, 0, 0);

					if (position > 0)
					{
						SendDlgItemMessage(hDlg, SLIDER_ANGLE, TBM_SETPOS, TRUE, --position);

						Joint::settings[GUI::checked_joint_id].now = 1800 - position;
						::setJointSettingNow(hDlg);

						if (BGAPI::connected)
						{
							std::string cmd = "#SA" + ::buildCmd(hDlg);
							ble_cmd_attclient_attribute_write(0, 31, 8, cmd.c_str());
							Sleep(10);

							BGAPI::readMessage();
							BGAPI::readMessage();
						}
					}

					break;
				}

				case BUTTON_MAX:
				{
					if (BGAPI::connected)
					{
						Joint::settings[GUI::checked_joint_id].max = 1800 - SendDlgItemMessage(hDlg, SLIDER_ANGLE, TBM_GETPOS, 0, 0);
						::setJointSettingMax(hDlg);

						std::string cmd = "#MA" + ::buildCmd(hDlg);
						ble_cmd_attclient_attribute_write(0, 31, 8, cmd.c_str());
						Sleep(10);

						BGAPI::readMessage();
						BGAPI::readMessage();
					}

					break;
				}

				case BUTTON_MIN:
				{
					if (BGAPI::connected)
					{
						Joint::settings[GUI::checked_joint_id].min = 1800 - SendDlgItemMessage(hDlg, SLIDER_ANGLE, TBM_GETPOS, 0, 0);
						::setJointSettingMin(hDlg);

						std::string cmd = "#MI" + ::buildCmd(hDlg);
						ble_cmd_attclient_attribute_write(0, 31, 8, cmd.c_str());
						Sleep(10);

						BGAPI::readMessage();
						BGAPI::readMessage();
					}

					break;
				}

				case BUTTON_HOME:
				{
					if (BGAPI::connected)
					{
						Joint::settings[GUI::checked_joint_id].home = 1800 - SendDlgItemMessage(hDlg, SLIDER_ANGLE, TBM_GETPOS, 0, 0);
						::setJointSettingHome(hDlg);

						std::string cmd = "#HO" + ::buildCmd(hDlg);
						ble_cmd_attclient_attribute_write(0, 31, 8, cmd.c_str());
						Sleep(10);

						BGAPI::readMessage();
						BGAPI::readMessage();
					}

					break;
				}

				case BUTTON_COM_CONNECT:
				{
					if (BGAPI::handle_created)
					{
						if (BGAPI::connected)
						{
							ble_cmd_connection_disconnect(0);
							Sleep(10);
							BGAPI::readMessage();

							BGAPI::connected = false;
							SetDlgItemText(hDlg, EDIT_MAC, "");
						}

						CloseHandle(BGAPI::bled112_handle);

						BGAPI::handle_created = false;
					}

					char buff[256] = { '\0' };
					std::string com = "\\\\.\\";
					SendDlgItemMessage(hDlg, LIST_COM, LB_GETTEXT, SendDlgItemMessage(hDlg, LIST_COM, LB_GETCURSEL, 0, 0), (LPARAM)buff);
					
					com += buff;
					BGAPI::bled112_handle = CreateFileA(com.c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);

					if (BGAPI::bled112_handle == INVALID_HANDLE_VALUE)
					{
						MessageBox(NULL, "COM�|�[�g�̃I�[�v���Ɏ��s���܂����B", "Error.", MB_OK);

						break;
					}

					BGAPI::handle_created = true;
					MessageBox(NULL, "COM�|�[�g�̃I�[�v���ɐ������܂����B", "Success.", MB_OK);

					break;
				}

				case BUTTON_COM_DISCONNECT:
				{
					if (BGAPI::handle_created)
					{
						if (BGAPI::connected)
						{
							ble_cmd_connection_disconnect(0);
							Sleep(10);
							BGAPI::readMessage();

							BGAPI::connected = false;
							SetDlgItemText(hDlg, EDIT_MAC, "");
						}

						CloseHandle(BGAPI::bled112_handle);

						BGAPI::handle_created = false;

						MessageBox(NULL, "COM�|�[�g���N���[�Y���܂����B", "Success.", MB_OK);
					}				

					break;
				}

				case BUTTON_PLEN2_SCAN:
				{
					if (BGAPI::handle_created)
					{
						ble_cmd_gap_end_procedure();
						Sleep(10);
						BGAPI::readMessage();

						ble_cmd_connection_disconnect(0);
						Sleep(10);
						BGAPI::readMessage();

						ble_cmd_gap_discover(gap_discover_generic);
						Sleep(10);
						BGAPI::readMessage();

						// ble_evt_connection_status()�𔭐��񐔕�����
						while (!BGAPI::connected)
						{
							BGAPI::readMessage();
						}

						::loadJointSetting(hDlg, true);
					}

					break;
				}

				case BUTTON_PLEN2_DISCONNECT:
				{
					if (BGAPI::connected)
					{
						ble_cmd_connection_disconnect(0);
						Sleep(10);
						BGAPI::readMessage();

						BGAPI::connected = false;
						SetDlgItemText(hDlg, EDIT_MAC, "");

						MessageBox(NULL, "PLEN2�Ƃ̐ڑ����������܂����B", "Success.", MB_OK);
					}

					break;
				}

				default:
				{
					// ���W�I�{�^�����N���b�N���ꂽ�Ȃ�A����ID��ێ�����B
					if (   LOWORD(wp) >= RADIO_JOINT01
						&& LOWORD(wp) <= RADIO_JOINT18 )
					{
						GUI::checked_joint_id = (int)LPWORD(wp);
						GUI::checked_joint_id -= RADIO_JOINT01; // �L���X�g�΍�̂��߁A2�s�ɕ�����
						::loadJointSetting(hDlg);
					}

					break;
				}
			}

			return TRUE;
		}

		case WM_CLOSE:
		{
			int ret = MessageBox(NULL, "�{���ɃA�v���P�[�V�������I�����܂����H\n(�S�Ă̍�Ɨ������j������܂��B)", "�I���m�F", MB_OKCANCEL);

			if (ret == IDOK)
			{
				if (BGAPI::connected)
				{
					ble_cmd_connection_disconnect(0);
					Sleep(10);
					BGAPI::readMessage();
				}

				if (BGAPI::handle_created)
				{
					CloseHandle(BGAPI::bled112_handle);
				}

				EndDialog(hDlg, 0);
			}

			return TRUE;
		}

		default:
		{
			break;
		}
	}

	return FALSE;
}


int WINAPI WinMain(HINSTANCE hCurrInst, HINSTANCE hPrevInst, LPSTR lpsCmdLine, int nCmdShow)
{
	DialogBox(hCurrInst, MAKEINTRESOURCE(DIALOG_MAIN), NULL, (DLGPROC)mainDlgProc);

	return 0;
}