/**
    For conditions of distribution and use, see copyright notice in LICENSE
*/

// Applicable only in headful mode.
if (!framework.IsHeadless())
{
    engine.ImportExtension("qt.core");
    engine.ImportExtension("qt.gui");

    CreateMenu();
}

// Add menu entry to Settings menu
function CreateMenu()
{
    var viewMenu = findChild(ui.MainWindow().menuBar(), "ViewMenu");

    if (viewMenu == null)
    {
        print("Menu not created yet. Retrying");
        frame.DelayedExecute(1.0).Triggered.connect(CreateMenu);
        return;
    }
    if (framework.GetModuleByName("CAVEStereo"))
    {
        viewMenu.addAction("Cave").triggered.connect(OpenCaveWindow);
        viewMenu.addAction("Stereoscopy").triggered.connect(OpenStereoscopyWindow);
    }
}

function OpenStereoscopyWindow() {
    framework.GetModuleByName("CAVEStereo").ShowStereoscopyWindow();
}

function OpenCaveWindow() {
    framework.GetModuleByName("CAVEStereo").ShowCaveWindow();
}
