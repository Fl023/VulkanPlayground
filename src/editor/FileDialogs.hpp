#pragma once

#include <string>

class FileDialogs
{
public:
    // Öffnet einen "Datei öffnen" Dialog und gibt den absoluten Dateipfad zurück.
    // Der Filter bestimmt, welche Dateitypen angezeigt werden (z.B. "Images\0*.png;*.jpg\0All Files\0*.*\0")
    static std::string OpenFile(const char* filter);
};