# Fix corrupted emoji character in main.cpp
$content = Get-Content "src\main.cpp" -Raw -Encoding UTF8
$content = $content -replace "title='Packed'>[^`"<]*`"", "title='Packed'>\u{1F4E6}`""
$content | Set-Content "src\main.cpp" -Encoding UTF8
Write-Host "Fixed corrupted emoji characters"