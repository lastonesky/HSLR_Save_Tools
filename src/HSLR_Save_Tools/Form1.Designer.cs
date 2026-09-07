namespace HSLR_Save_Tools;

partial class Form1
{
    private System.ComponentModel.IContainer components = null;

    protected override void Dispose(bool disposing)
    {
        if (disposing && (components != null))
        {
            components.Dispose();
        }
        base.Dispose(disposing);
    }

    #region Windows Form Designer generated code

    private void InitializeComponent()
    {
        this.btnOpen = new System.Windows.Forms.Button();
        this.lblPath = new System.Windows.Forms.Label();
        this.cbChars = new System.Windows.Forms.ComboBox();
        this.label1 = new System.Windows.Forms.Label();
        this.tabControl1 = new System.Windows.Forms.TabControl();
        this.tpBasic = new System.Windows.Forms.TabPage();
        this.tpRecord = new System.Windows.Forms.TabPage();
        this.tpBattle = new System.Windows.Forms.TabPage();
        this.tpEquip = new System.Windows.Forms.TabPage();
        this.tpRoster = new System.Windows.Forms.TabPage();
        this.btnSave = new System.Windows.Forms.Button();
        this.btnRefresh = new System.Windows.Forms.Button();
        this.btnMaxAll = new System.Windows.Forms.Button();
        this.statusStrip1 = new System.Windows.Forms.StatusStrip();
        this.statusLabel = new System.Windows.Forms.ToolStripStatusLabel();
        
        this.tabControl1.SuspendLayout();
        this.statusStrip1.SuspendLayout();
        this.SuspendLayout();

        // btnOpen
        this.btnOpen.Location = new System.Drawing.Point(12, 12);
        this.btnOpen.Name = "btnOpen";
        this.btnOpen.Size = new System.Drawing.Size(120, 30);
        this.btnOpen.TabIndex = 0;
        this.btnOpen.Text = "打开存档 (.sav)";
        this.btnOpen.UseVisualStyleBackColor = true;
        this.btnOpen.Click += new System.EventHandler(this.btnOpen_Click);

        // lblPath
        this.lblPath.AutoSize = true;
        this.lblPath.ForeColor = System.Drawing.Color.Gray;
        this.lblPath.Location = new System.Drawing.Point(138, 20);
        this.lblPath.Name = "lblPath";
        this.lblPath.Size = new System.Drawing.Size(104, 17);
        this.lblPath.TabIndex = 1;
        this.lblPath.Text = "请先打开存档文件";

        // label1
        this.label1.AutoSize = true;
        this.label1.Location = new System.Drawing.Point(400, 20);
        this.label1.Name = "label1";
        this.label1.Size = new System.Drawing.Size(59, 17);
        this.label1.TabIndex = 2;
        this.label1.Text = "当前角色:";

        // cbChars
        this.cbChars.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
        this.cbChars.FormattingEnabled = true;
        this.cbChars.Location = new System.Drawing.Point(465, 17);
        this.cbChars.Name = "cbChars";
        this.cbChars.Size = new System.Drawing.Size(240, 25);
        this.cbChars.TabIndex = 3;
        this.cbChars.SelectedIndexChanged += new System.EventHandler(this.cbChars_SelectedIndexChanged);

        // tabControl1
        this.tabControl1.Controls.Add(this.tpBasic);
        this.tabControl1.Controls.Add(this.tpRecord);
        this.tabControl1.Controls.Add(this.tpBattle);
        this.tabControl1.Controls.Add(this.tpEquip);
        this.tabControl1.Controls.Add(this.tpRoster);
        this.tabControl1.Location = new System.Drawing.Point(12, 50);
        this.tabControl1.Name = "tabControl1";
        this.tabControl1.SelectedIndex = 0;
        this.tabControl1.Size = new System.Drawing.Size(800, 700);
        this.tabControl1.TabIndex = 4;

        // tpBasic
        this.tpBasic.Location = new System.Drawing.Point(4, 26);
        this.tpBasic.Name = "tpBasic";
        this.tpBasic.Padding = new System.Windows.Forms.Padding(3);
        this.tpBasic.Size = new System.Drawing.Size(692, 670);
        this.tpBasic.TabIndex = 0;
        this.tpBasic.Text = "基础信息";
        this.tpBasic.UseVisualStyleBackColor = true;

        // tpRecord
        this.tpRecord.Location = new System.Drawing.Point(4, 26);
        this.tpRecord.Name = "tpRecord";
        this.tpRecord.Padding = new System.Windows.Forms.Padding(3);
        this.tpRecord.Size = new System.Drawing.Size(692, 670);
        this.tpRecord.TabIndex = 1;
        this.tpRecord.Text = "存档属性";
        this.tpRecord.UseVisualStyleBackColor = true;

        // tpBattle
        this.tpBattle.Location = new System.Drawing.Point(4, 26);
        this.tpBattle.Name = "tpBattle";
        this.tpBattle.Size = new System.Drawing.Size(692, 670);
        this.tpBattle.TabIndex = 2;
        this.tpBattle.Text = "战场属性";
        this.tpBattle.UseVisualStyleBackColor = true;

        // tpEquip
        this.tpEquip.Location = new System.Drawing.Point(4, 26);
        this.tpEquip.Name = "tpEquip";
        this.tpEquip.Size = new System.Drawing.Size(692, 670);
        this.tpEquip.TabIndex = 3;
        this.tpEquip.Text = "装备/道具/技能";
        this.tpEquip.UseVisualStyleBackColor = true;

        // tpRoster
        this.tpRoster.Location = new System.Drawing.Point(4, 26);
        this.tpRoster.Name = "tpRoster";
        this.tpRoster.Size = new System.Drawing.Size(692, 670);
        this.tpRoster.TabIndex = 4;
        this.tpRoster.Text = "全角色一览";
        this.tpRoster.UseVisualStyleBackColor = true;

        // btnSave
        this.btnSave.Location = new System.Drawing.Point(712, 760);
        this.btnSave.Name = "btnSave";
        this.btnSave.Size = new System.Drawing.Size(100, 30);
        this.btnSave.TabIndex = 5;
        this.btnSave.Text = "💾 保存存档";
        this.btnSave.UseVisualStyleBackColor = true;
        this.btnSave.Click += new System.EventHandler(this.btnSave_Click);

        // btnRefresh
        this.btnRefresh.Location = new System.Drawing.Point(606, 760);
        this.btnRefresh.Name = "btnRefresh";
        this.btnRefresh.Size = new System.Drawing.Size(100, 30);
        this.btnRefresh.TabIndex = 6;
        this.btnRefresh.Text = "🔄 刷新显示";
        this.btnRefresh.UseVisualStyleBackColor = true;
        this.btnRefresh.Click += new System.EventHandler(this.btnRefresh_Click);

        // btnMaxAll
        this.btnMaxAll.Location = new System.Drawing.Point(12, 760);
        this.btnMaxAll.Name = "btnMaxAll";
        this.btnMaxAll.Size = new System.Drawing.Size(150, 30);
        this.btnMaxAll.TabIndex = 7;
        this.btnMaxAll.Text = "⚡ 全队满属性(持久)";
        this.btnMaxAll.UseVisualStyleBackColor = true;
        this.btnMaxAll.Click += new System.EventHandler(this.btnMaxAll_Click);

        // statusStrip1
        this.statusStrip1.Items.AddRange(new System.Windows.Forms.ToolStripItem[] { this.statusLabel });
        this.statusStrip1.Location = new System.Drawing.Point(0, 800);
        this.statusStrip1.Name = "statusStrip1";
        this.statusStrip1.Size = new System.Drawing.Size(724, 22);
        this.statusStrip1.TabIndex = 8;
        this.statusStrip1.Text = "statusStrip1";

        // statusLabel
        this.statusLabel.Name = "statusLabel";
        this.statusLabel.Size = new System.Drawing.Size(32, 17);
        this.statusLabel.Text = "就绪";

        // Form1
        this.AutoScaleDimensions = new System.Drawing.SizeF(7F, 17F);
        this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
        this.ClientSize = new System.Drawing.Size(824, 822);
        this.Controls.Add(this.btnMaxAll);
        this.Controls.Add(this.btnRefresh);
        this.Controls.Add(this.btnSave);
        this.Controls.Add(this.tabControl1);
        this.Controls.Add(this.cbChars);
        this.Controls.Add(this.label1);
        this.Controls.Add(this.lblPath);
        this.Controls.Add(this.btnOpen);
        this.Controls.Add(this.statusStrip1);
        this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedSingle;
        this.MaximizeBox = false;
        this.Name = "Form1";
        this.Text = "幻世录重制版 存档编辑器 v2 (C# Port)";
        this.tabControl1.ResumeLayout(false);
        this.statusStrip1.ResumeLayout(false);
        this.statusStrip1.PerformLayout();
        this.ResumeLayout(false);
        this.PerformLayout();
    }

    #endregion

    private System.Windows.Forms.Button btnOpen;
    private System.Windows.Forms.Label lblPath;
    private System.Windows.Forms.ComboBox cbChars;
    private System.Windows.Forms.Label label1;
    private System.Windows.Forms.TabControl tabControl1;
    private System.Windows.Forms.TabPage tpBasic;
    private System.Windows.Forms.TabPage tpRecord;
    private System.Windows.Forms.TabPage tpBattle;
    private System.Windows.Forms.TabPage tpEquip;
    private System.Windows.Forms.TabPage tpRoster;
    private System.Windows.Forms.Button btnSave;
    private System.Windows.Forms.Button btnRefresh;
    private System.Windows.Forms.Button btnMaxAll;
    private System.Windows.Forms.StatusStrip statusStrip1;
    private System.Windows.Forms.ToolStripStatusLabel statusLabel;
}
