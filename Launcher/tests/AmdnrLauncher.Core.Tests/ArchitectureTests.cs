// Copyright (c) 2026 3zwr1 (AMDNR). All rights reserved. Source published to be read, not reused: see LICENSE.txt.
using System.Reflection;

namespace AmdnrLauncher.Core.Tests;

public class ArchitectureTests
{
    [Fact]
    public void Core_does_not_reference_any_UI_assembly()
    {
        var referenced = typeof(Hashing).Assembly
            .GetReferencedAssemblies()
            .Select(a => a.Name ?? string.Empty)
            .ToArray();

        Assert.DoesNotContain(referenced, n =>
            n.StartsWith("PresentationFramework", StringComparison.OrdinalIgnoreCase) ||
            n.StartsWith("PresentationCore", StringComparison.OrdinalIgnoreCase) ||
            n.StartsWith("WindowsBase", StringComparison.OrdinalIgnoreCase) ||
            n.StartsWith("System.Windows.Forms", StringComparison.OrdinalIgnoreCase));
    }
}
