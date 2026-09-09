// 幻世录重制版存档编辑器 - Rust TUI版本
mod crypto;
mod editor;
mod save;
mod ui;

use std::io;
use std::time::Duration;

use crossterm::{
    event::{self, Event, KeyCode, KeyEventKind, KeyModifiers},
    execute,
    terminal::{disable_raw_mode, enable_raw_mode, EnterAlternateScreen, LeaveAlternateScreen},
};
use ratatui::prelude::CrosstermBackend;
use ratatui::Terminal;

fn main() -> anyhow::Result<()> {
    // 设置终端
    enable_raw_mode()?;
    let mut stdout = io::stdout();
    execute!(stdout, EnterAlternateScreen)?;
    let backend = CrosstermBackend::new(stdout);
    let mut terminal = Terminal::new(backend)?;

    // 创建应用
    let mut app = editor::EditorApp::new();

    // 如果命令行提供了文件路径，自动打开
    let args: Vec<String> = std::env::args().collect();
    if args.len() > 1 {
        let path = std::path::PathBuf::from(&args[1]);
        if path.exists() {
            if let Err(e) = app.open_file(path) {
                app.status_message = format!("✗ 加载失败: {}", e);
                app.status_is_error = true;
            } else {
                app.build_edit_fields();
            }
        }
    }

    // 主循环
    let result = run_app(&mut terminal, &mut app);

    // 恢复终端
    disable_raw_mode()?;
    execute!(terminal.backend_mut(), LeaveAlternateScreen)?;
    terminal.show_cursor()?;

    if let Err(err) = result {
        eprintln!("错误: {}", err);
    }

    Ok(())
}

fn run_app(
    terminal: &mut Terminal<CrosstermBackend<io::Stdout>>,
    app: &mut editor::EditorApp,
) -> anyhow::Result<()> {
    loop {
        // 绘制UI
        terminal.draw(|f| ui::draw(f, app))?;

        // 处理事件
        if event::poll(Duration::from_millis(100))? {
            if let Event::Key(key) = event::read()? {
                if key.kind == KeyEventKind::Press {
                    match key.code {
                        // 退出
                        KeyCode::Char('q') | KeyCode::Char('Q') => {
                            if !app.editing {
                                return Ok(());
                            }
                        }

                        // 切换标签页 (非编辑模式)
                        KeyCode::Tab if !app.editing => {
                            app.next_tab();
                            app.build_edit_fields();
                        }
                        KeyCode::BackTab if !app.editing => {
                            app.prev_tab();
                            app.build_edit_fields();
                        }
                        KeyCode::Char('1') if !app.editing => { app.switch_tab(0); }
                        KeyCode::Char('2') if !app.editing => { app.switch_tab(1); }
                        KeyCode::Char('3') if !app.editing => { app.switch_tab(2); }
                        KeyCode::Char('4') if !app.editing => { app.switch_tab(3); }
                        KeyCode::Char('5') if !app.editing => { app.switch_tab(4); }

                        // 打开文件
                        KeyCode::Char('o') | KeyCode::Char('O') => {
                            if key.modifiers.contains(KeyModifiers::CONTROL) {
                                open_file_dialog(app);
                            }
                        }

                        // 保存文件
                        KeyCode::Char('s') | KeyCode::Char('S') => {
                            if key.modifiers.contains(KeyModifiers::CONTROL) {
                                if let Err(e) = app.save_file() {
                                    app.status_message = format!("✗ 保存失败: {}", e);
                                    app.status_is_error = true;
                                }
                            }
                        }

                        // F2: 全队满属性
                        KeyCode::F(2) => {
                            app.batch_max_all();
                        }

                        // F3: 保存
                        KeyCode::F(3) => {
                            if let Err(e) = app.save_file() {
                                app.status_message = format!("✗ 保存失败: {}", e);
                                app.status_is_error = true;
                            }
                        }

                        // F5: 刷新
                        KeyCode::F(5) => {
                            app.status_message = "已刷新".to_string();
                            app.status_is_error = false;
                        }

                        // 战场属性页快捷键
                        KeyCode::Char('h') | KeyCode::Char('H') => {
                            if app.current_tab == 2 {
                                app.full_heal();
                            }
                        }
                        KeyCode::Char('m') | KeyCode::Char('M') => {
                            if app.current_tab == 2 {
                                app.max_stats_battle();
                            }
                        }
                        KeyCode::Char('l') | KeyCode::Char('L') => {
                            if app.current_tab == 2 {
                                app.max_level();
                            }
                        }

                        // 角色切换 (上/下箭头) - 仅在非编辑模式
                        KeyCode::Up => {
                            if app.editing {
                                // 编辑模式下不处理
                            } else if !app.edit_fields.is_empty() {
                                app.prev_field();
                            } else if !app.char_list.is_empty() {
                                let current_idx = app
                                    .char_list
                                    .iter()
                                    .position(|(pid, _, _)| Some(*pid) == app.current_pid)
                                    .unwrap_or(0);
                                let new_idx = if current_idx == 0 {
                                    app.char_list.len() - 1
                                } else {
                                    current_idx - 1
                                };
                                app.select_char(app.char_list[new_idx].0);
                            }
                        }
                        KeyCode::Down => {
                            if app.editing {
                                // 编辑模式下不处理
                            } else if !app.edit_fields.is_empty() {
                                app.next_field();
                            } else if !app.char_list.is_empty() {
                                let current_idx = app
                                    .char_list
                                    .iter()
                                    .position(|(pid, _, _)| Some(*pid) == app.current_pid)
                                    .unwrap_or(0);
                                let new_idx = (current_idx + 1) % app.char_list.len();
                                app.select_char(app.char_list[new_idx].0);
                            }
                        }

                        // Enter: 开始编辑 / 确认编辑
                        KeyCode::Enter => {
                            if app.editing {
                                app.confirm_edit();
                            } else if !app.edit_fields.is_empty() {
                                app.start_edit();
                            }
                        }

                        // Esc: 取消编辑
                        KeyCode::Esc => {
                            if app.editing {
                                app.cancel_edit();
                            } else {
                                return Ok(());
                            }
                        }

                        // +/-: 快捷增减 (仅非编辑模式)
                        KeyCode::Char('+') | KeyCode::Char('=') => {
                            if !app.editing {
                                app.increment_value(1);
                            }
                        }
                        KeyCode::Char('-') | KeyCode::Char('_') => {
                            if !app.editing {
                                app.increment_value(-1);
                            }
                        }

                        // 编辑模式下的数字和符号输入
                        KeyCode::Char(c) if app.editing && (c.is_ascii_digit() || c == '-' || (c == ',' && app.items_edit_mode)) => {
                            app.input_buffer.push(c);
                        }
                        KeyCode::Backspace if app.editing => {
                            app.input_buffer.pop();
                        }

                        _ => {}
                    }
                }
            }
        }
    }
}

/// 打开文件对话框
fn open_file_dialog(app: &mut editor::EditorApp) {
    let default_dir = get_default_save_dir();

    let dialog = rfd::FileDialog::new()
        .set_title("打开存档文件")
        .add_filter("SAV files", &["sav"])
        .add_filter("All files", &["*"]);

    let dialog = if let Some(dir) = default_dir {
        dialog.set_directory(dir)
    } else {
        dialog
    };

    if let Some(path) = dialog.pick_file() {
        if let Err(e) = app.open_file(path) {
            app.status_message = format!("✗ 加载失败: {}", e);
            app.status_is_error = true;
        } else {
            app.build_edit_fields();
        }
    }
}

/// 获取默认存档目录
fn get_default_save_dir() -> Option<std::path::PathBuf> {
    let user_profile = std::env::var("USERPROFILE").ok()?;
    let save_dir = std::path::PathBuf::from(user_profile)
        .join("AppData")
        .join("LocalLow")
        .join("UserJoy")
        .join("HSLR")
        .join("Save")
        .join("Save_Demo")
        .join("sav");

    if save_dir.is_dir() {
        Some(save_dir)
    } else {
        None
    }
}
