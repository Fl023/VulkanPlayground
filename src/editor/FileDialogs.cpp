#include "FileDialogs.hpp"
#include <windows.h>
#include <commdlg.h> // Für GetOpenFileNameA

std::string FileDialogs::OpenFile(const char* filter)
{
    OPENFILENAMEA ofn;
    CHAR szFile[260] = { 0 };

    ZeroMemory(&ofn, sizeof(OPENFILENAME));
    ofn.lStructSize = sizeof(OPENFILENAME);
    
    // Wenn du Zugriff auf die native HWND deines Fensters hast, könntest du sie hier übergeben.
    // nullptr funktioniert aber auch problemlos (der Dialog ist dann nur nicht streng an die Engine gebunden).
    ofn.hwndOwner = nullptr; 
    
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = filter;
    ofn.nFilterIndex = 1;
    
    // OFN_NOCHANGEDIR ist EXTREM WICHTIG! 
    // Ohne dieses Flag ändert Windows heimlich das Working Directory deiner Engine 
    // auf den Ordner, in dem du das Bild auswählst. Das zerschießt dir alle anderen relativen Pfade!
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

    if (GetOpenFileNameA(&ofn) == TRUE)
    {
        return ofn.lpstrFile;
    }

    return std::string(); // Leerer String, wenn der User auf "Abbrechen" drückt
}

std::string FileDialogs::SaveFile(const char* filter)
{
    OPENFILENAMEA ofn;
    CHAR szFile[260] = { 0 };

    ZeroMemory(&ofn, sizeof(OPENFILENAME));
    ofn.lStructSize = sizeof(OPENFILENAME);

    ofn.hwndOwner = nullptr;

    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = filter;
    ofn.nFilterIndex = 1;

    // OFN_OVERWRITEPROMPT pops up a warning if the user selects an existing file to overwrite!
    // OFN_FILEMUSTEXIST is removed because we are creating a new file.
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;

    // Use GetSaveFileNameA instead of GetOpenFileNameA
    if (GetSaveFileNameA(&ofn) == TRUE)
    {
        return ofn.lpstrFile;
    }

    return std::string(); // Returns empty string if the user clicks "Cancel"
}