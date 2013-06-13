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

    if (framework.ModuleByName("CAVEStereo"))
    {
        viewMenu.addAction("Cave").triggered.connect(OpenCaveWindow);
        viewMenu.addAction("Stereoscopy").triggered.connect(OpenStereoscopyWindow);
    }

    if (framework.ModuleByName("PythonScript"))
        viewMenu.addAction("Python Console").triggered.connect(OpenPythonConsole);
}

function OpenStereoscopyWindow()
{
    framework.ModuleByName("CAVEStereo").ShowStereoscopyWindow();
}

function OpenCaveWindow()
{
    framework.ModuleByName("CAVEStereo").ShowCaveWindow();
}

function OpenPythonConsole()
{
    framework.ModuleByName("PythonScript").ShowConsole();
}
