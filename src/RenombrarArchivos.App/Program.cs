using System.ComponentModel;
using System.Runtime.InteropServices;
using System.Text;

namespace RenombrarArchivos.App;

internal static class Program
{
    [STAThread]
    private static void Main()
    {
        ApplicationConfiguration.Initialize();
        Application.Run(new MainForm());
    }
}

internal sealed class FilePlan
{
    public required string OriginalPath { get; init; }
    public required string OriginalName { get; init; }
    public required string ProposedName { get; init; }
    public required string Status { get; init; }
    public long? Number { get; init; }
    public bool HasNumber { get; init; }
    public bool HasChange => !string.Equals(OriginalName, ProposedName, StringComparison.Ordinal);
}

internal static class NativeCore
{
    [DllImport("RenombrarArchivos.Core.dll", EntryPoint = "NormalizeNumberPrefix",
        CharSet = CharSet.Unicode, CallingConvention = CallingConvention.Cdecl)]
    private static extern int NormalizeNumberPrefix(
        [MarshalAs(UnmanagedType.LPWStr)] string stem,
        StringBuilder output,
        int outputCount);

    public static string? Normalize(string stem)
    {
        var buffer = new StringBuilder(32768);
        return NormalizeNumberPrefix(stem, buffer, buffer.Capacity) == 1
            ? buffer.ToString()
            : null;
    }
}

internal sealed class MainForm : Form
{
    private readonly TextBox folderBox = new();
    private readonly DataGridView grid = new();
    private readonly Label statusLabel = new();
    private readonly Button applyButton = new();
    private readonly BindingList<FilePlan> plans = new();
    private string? selectedFolder;

    public MainForm()
    {
        Text = "Renombrar archivos";
        StartPosition = FormStartPosition.CenterScreen;
        MinimumSize = new Size(800, 480);
        Size = new Size(1050, 650);
        Font = new Font("Segoe UI", 9F);
        BackColor = Color.FromArgb(246, 248, 251);

        var header = new Panel { Dock = DockStyle.Top, Height = 124, Padding = new Padding(18, 14, 18, 8) };
        var title = new Label
        {
            Text = "Renombrar archivos",
            Font = new Font("Segoe UI", 19F, FontStyle.Bold),
            AutoSize = true,
            Location = new Point(18, 12)
        };
        var subtitle = new Label
        {
            Text = "El número se obtiene del nombre original, nunca de la posición en la lista. Ejemplo: 8 → 008.",
            AutoSize = true,
            ForeColor = Color.FromArgb(80, 91, 105),
            Location = new Point(20, 52)
        };
        folderBox.ReadOnly = true;
        folderBox.Anchor = AnchorStyles.Top | AnchorStyles.Left | AnchorStyles.Right;
        folderBox.Location = new Point(18, 82);
        folderBox.Width = 650;
        folderBox.Height = 30;
        folderBox.BackColor = Color.White;
        var chooseButton = new Button { Text = "Elegir carpeta…", Width = 130, Height = 30, Anchor = AnchorStyles.Top | AnchorStyles.Right };
        chooseButton.Location = new Point(680, 81);
        chooseButton.Click += (_, _) => ChooseFolder();
        var refreshButton = new Button { Text = "Actualizar", Width = 95, Height = 30, Anchor = AnchorStyles.Top | AnchorStyles.Right };
        refreshButton.Location = new Point(818, 81);
        refreshButton.Click += (_, _) => ScanFolder();
        header.Controls.AddRange([title, subtitle, folderBox, chooseButton, refreshButton]);
        header.Resize += (_, _) =>
        {
            folderBox.Width = Math.Max(100, header.ClientSize.Width - 365);
            chooseButton.Location = new Point(header.ClientSize.Width - 322, 81);
            refreshButton.Location = new Point(header.ClientSize.Width - 184, 81);
        };

        grid.Dock = DockStyle.Fill;
        grid.ReadOnly = true;
        grid.AllowUserToAddRows = false;
        grid.AllowUserToDeleteRows = false;
        grid.AllowUserToResizeRows = false;
        grid.MultiSelect = false;
        grid.RowHeadersVisible = false;
        grid.AutoGenerateColumns = false;
        grid.BackgroundColor = Color.White;
        grid.BorderStyle = BorderStyle.FixedSingle;
        grid.SelectionMode = DataGridViewSelectionMode.FullRowSelect;
        grid.AutoSizeColumnsMode = DataGridViewAutoSizeColumnsMode.Fill;
        grid.Columns.Add(new DataGridViewTextBoxColumn { HeaderText = "Nombre original", DataPropertyName = nameof(FilePlan.OriginalName), FillWeight = 100 });
        grid.Columns.Add(new DataGridViewTextBoxColumn { HeaderText = "Nombre propuesto", DataPropertyName = nameof(FilePlan.ProposedName), FillWeight = 100 });
        grid.Columns.Add(new DataGridViewTextBoxColumn { HeaderText = "Estado", DataPropertyName = nameof(FilePlan.Status), FillWeight = 42 });
        grid.DataSource = plans;

        var footer = new Panel { Dock = DockStyle.Bottom, Height = 72, Padding = new Padding(18, 12, 18, 12) };
        statusLabel.Dock = DockStyle.Fill;
        statusLabel.TextAlign = ContentAlignment.MiddleLeft;
        statusLabel.Text = "Elige una carpeta para ver la vista previa. No se cambia nada hasta confirmar.";
        statusLabel.AutoEllipsis = true;
        applyButton.Text = "Renombrar archivos";
        applyButton.Width = 170;
        applyButton.Dock = DockStyle.Right;
        applyButton.Enabled = false;
        applyButton.Click += (_, _) => ApplyChanges();
        footer.Controls.Add(statusLabel);
        footer.Controls.Add(applyButton);

        Controls.Add(grid);
        Controls.Add(footer);
        Controls.Add(header);
    }

    private void ChooseFolder()
    {
        using var dialog = new FolderBrowserDialog
        {
            Description = "Selecciona la carpeta que contiene los archivos",
            UseDescriptionForTitle = true,
            ShowNewFolderButton = false
        };
        if (dialog.ShowDialog(this) != DialogResult.OK) return;
        selectedFolder = dialog.SelectedPath;
        folderBox.Text = selectedFolder;
        ScanFolder();
    }

    private void ScanFolder()
    {
        if (string.IsNullOrWhiteSpace(selectedFolder) || !Directory.Exists(selectedFolder))
        {
            MessageBox.Show(this, "Primero selecciona una carpeta válida.", "Renombrar archivos",
                MessageBoxButtons.OK, MessageBoxIcon.Information);
            return;
        }

        try
        {
            plans.Clear();
            var found = new List<FilePlan>();
            foreach (var path in Directory.EnumerateFiles(selectedFolder, "*", SearchOption.TopDirectoryOnly))
            {
                var name = Path.GetFileName(path);
                var stem = Path.GetFileNameWithoutExtension(name);
                var normalized = NativeCore.Normalize(stem);
                if (normalized is null)
                {
                    found.Add(new FilePlan
                    {
                        OriginalPath = path, OriginalName = name, ProposedName = name,
                        Status = "Sin número: intacto", Number = null, HasNumber = false
                    });
                    continue;
                }

                var digits = new string(stem.TrimStart().TakeWhile(char.IsDigit).ToArray());
                long? number = long.TryParse(digits, out var parsed) ? parsed : null;
                var proposed = normalized + Path.GetExtension(name);
                found.Add(new FilePlan
                {
                    OriginalPath = path,
                    OriginalName = name,
                    ProposedName = proposed,
                    Status = string.Equals(name, proposed, StringComparison.Ordinal) ? "Ya correcto" : "Listo para cambiar",
                    Number = number,
                    HasNumber = true
                });
            }

            foreach (var item in found.OrderBy(p => !p.HasNumber)
                         .ThenBy(p => p.Number ?? long.MaxValue)
                         .ThenBy(p => p.OriginalName, StringComparer.OrdinalIgnoreCase))
                plans.Add(item);

            var changes = plans.Count(p => p.HasChange);
            var skipped = plans.Count(p => !p.HasNumber);
            statusLabel.Text = $"{plans.Count} archivos encontrados · {changes} cambios propuestos · {skipped} sin número (se dejan intactos).";
            applyButton.Enabled = changes > 0;
        }
        catch (DllNotFoundException)
        {
            MessageBox.Show(this,
                "No se encuentra RenombrarArchivos.Core.dll. Compila el proyecto con build.ps1 para copiar el motor C++ junto a la aplicación.",
                "Falta el motor C++", MessageBoxButtons.OK, MessageBoxIcon.Error);
        }
        catch (Exception ex)
        {
            MessageBox.Show(this, $"No se pudo leer la carpeta:\n{ex.Message}", "Error",
                MessageBoxButtons.OK, MessageBoxIcon.Error);
        }
    }

    private void ApplyChanges()
    {
        var changes = plans.Where(p => p.HasChange).ToList();
        if (changes.Count == 0 || selectedFolder is null) return;

        var comparer = StringComparer.OrdinalIgnoreCase;
        var targets = changes.Select(p => Path.Combine(selectedFolder, p.ProposedName)).ToList();
        if (targets.Select(Path.GetFileName).Distinct(comparer).Count() != targets.Count)
        {
            MessageBox.Show(this, "Dos archivos acabarían con el mismo nombre. No se ha cambiado nada.",
                "Conflicto de nombres", MessageBoxButtons.OK, MessageBoxIcon.Warning);
            return;
        }

        var sourceSet = new HashSet<string>(changes.Select(p => p.OriginalPath), comparer);
        foreach (var target in targets)
        {
            if (File.Exists(target) && !sourceSet.Contains(target))
            {
                MessageBox.Show(this, $"Ya existe un archivo con este nombre:\n{Path.GetFileName(target)}\n\nNo se ha cambiado nada.",
                    "Conflicto de nombres", MessageBoxButtons.OK, MessageBoxIcon.Warning);
                return;
            }
        }

        var preview = string.Join(Environment.NewLine, changes.Take(10)
            .Select(p => $"{p.OriginalName}  →  {p.ProposedName}"));
        if (changes.Count > 10) preview += $"{Environment.NewLine}… y {changes.Count - 10} cambios más";
        var answer = MessageBox.Show(this,
            $"Se van a renombrar {changes.Count} archivos en:\n{selectedFolder}\n\n{preview}\n\nNo se borrará ningún archivo. ¿Confirmas?",
            "Confirmar renombrado", MessageBoxButtons.YesNo, MessageBoxIcon.Question);
        if (answer != DialogResult.Yes) return;

        var staged = new List<(FilePlan Plan, string TempPath)>();
        var completed = new List<(FilePlan Plan, string FinalPath)>();
        try
        {
            foreach (var plan in changes)
            {
                var temp = Path.Combine(selectedFolder, $".renombrar_tmp_{Guid.NewGuid():N}{Path.GetExtension(plan.OriginalName)}");
                File.Move(plan.OriginalPath, temp);
                staged.Add((plan, temp));
            }

            foreach (var entry in staged.ToList())
            {
                var target = Path.Combine(selectedFolder, entry.Plan.ProposedName);
                if (File.Exists(target)) throw new IOException($"El destino ya existe: {entry.Plan.ProposedName}");
                File.Move(entry.TempPath, target);
                staged.Remove(entry);
                completed.Add((entry.Plan, target));
            }

            MessageBox.Show(this, $"Listo: {changes.Count} archivos renombrados.", "Terminado",
                MessageBoxButtons.OK, MessageBoxIcon.Information);
            ScanFolder();
        }
        catch (Exception ex)
        {
            // Best-effort rollback to original names; never overwrite an existing file.
            foreach (var entry in completed.AsEnumerable().Reverse())
            {
                try
                {
                    if (!File.Exists(entry.Plan.OriginalPath))
                        File.Move(entry.FinalPath, entry.Plan.OriginalPath);
                }
                catch { /* Keep the error visible; do not delete or overwrite anything. */ }
            }
            foreach (var entry in staged.AsEnumerable().Reverse())
            {
                try
                {
                    if (File.Exists(entry.TempPath) && !File.Exists(entry.Plan.OriginalPath))
                        File.Move(entry.TempPath, entry.Plan.OriginalPath);
                }
                catch { /* Keep the error visible; do not delete or overwrite anything. */ }
            }

            MessageBox.Show(this,
                $"No se pudo completar el renombrado. Se intentó restaurar los nombres originales.\n\nDetalle: {ex.Message}",
                "Operación incompleta", MessageBoxButtons.OK, MessageBoxIcon.Error);
            ScanFolder();
        }
    }
}
