# Brand pictures

Drop the owner's pictures here and rebuild: the csproj compiles every `*.png` in this folder
into the exe, and `Brand.cs` finds them by these exact names.

- `logo.png` - the square mark. Stands in for the drawn arrow in the wordmark on the chooser and in ABOUT.
- `wordmark.png` - the line "NEURAL RENDERING - AMD". Stands in for the drawn wordmark at the head of the chooser.

Until they are here, `BrandWordmark` draws the mark. Nothing generates these files.
