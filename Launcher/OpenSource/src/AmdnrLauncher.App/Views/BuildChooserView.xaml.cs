// Copyright (c) 2026 3zwr1 (AMDNR). All rights reserved. Source published to be read, not reused: see LICENSE.txt.
using System.Windows.Controls;
using System.Windows.Navigation;

namespace AmdnrLauncher.App.Views;

public partial class BuildChooserView : UserControl
{
    public BuildChooserView() => InitializeComponent();

    /// <summary>Every link on the page goes through Links, which opens web addresses only: a
    /// homepage comes from the manifest, and the shell would run whatever else it was handed.</summary>
    private void OnLink(object sender, RequestNavigateEventArgs e)
    {
        e.Handled = true;
        Services.Links.Open(e.Uri);
    }
}
