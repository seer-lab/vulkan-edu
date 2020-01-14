//#include <Windows.h>
//
//// MS-Windows event handling function:
//LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
//	struct sample_info* info = reinterpret_cast<struct sample_info*>(
//		GetWindowLongPtr(hWnd, GWLP_USERDATA));
//	switch (uMsg) {
//	case WM_CLOSE:
//		PostQuitMessage(0);
//		break;
//	case WM_PAINT:
//		//Todo Add a run function for dynamic stuff
//		//run(info);
//		return 0;
//	default:
//		break;
//	}
//	return (DefWindowProc(hWnd, uMsg, wParam, lParam));
//}