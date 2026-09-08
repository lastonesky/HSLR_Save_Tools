use ratatui::Frame;
use ratatui::layout::{Constraint, Direction, Layout, Rect};
use ratatui::style::{Color, Modifier, Style};
use ratatui::widgets::{Block, Borders, Paragraph};

use crate::editor::EditorApp;
use crate::save::*;

/// 绘制存档属性页
pub fn draw(f: &mut Frame, app: &EditorApp, area: Rect) {
    let chunks = Layout::default()
        .direction(Direction::Vertical)
        .constraints([
            Constraint::Length(3),  // 说明
            Constraint::Length(8),  // BaseAttr
            Constraint::Length(10), // PermanentFightAttr
            Constraint::Min(8),    // FightAttr
        ])
        .split(area);

    // 说明
    draw_note(f, chunks[0]);

    // BaseAttr (基础属性点，永久生效)
    draw_base_attr(f, app, chunks[1]);

    // PermanentFightAttr (永久加成，永久生效)
    draw_permanent_attr(f, app, chunks[2]);

    // FightAttr (战斗属性，只读参考)
    draw_fight_attr(f, app, chunks[3]);
}

/// 检查字段是否被选中
fn is_field_selected(app: &EditorApp, path: &[&str]) -> bool {
    if let Some(field) = app.edit_fields.get(app.selected_field) {
        let field_path: Vec<&str> = field.path.iter().map(|s| s.as_str()).collect();
        return field_path == path;
    }
    false
}

/// 格式化字段值 (高亮选中字段)
fn format_field_value(app: &EditorApp, path: &[&str], value: Option<i64>) -> (String, Style) {
    let val_str = value.map_or("-".to_string(), |v| v.to_string());
    let is_selected = is_field_selected(app, path);
    
    if app.editing && is_selected {
        // 编辑模式 - 显示输入缓冲区
        (app.input_buffer.clone(), Style::default().fg(Color::Black).bg(Color::Yellow))
    } else if is_selected {
        // 选中但未编辑
        (val_str, Style::default().fg(Color::Black).bg(Color::Cyan))
    } else {
        (val_str, Style::default().fg(Color::Yellow))
    }
}

/// 绘制说明
fn draw_note(f: &mut Frame, area: Rect) {
    let note = Paragraph::new("💡 永久加成和基础属性会被保留；战斗属性每次加载自动重算")
        .style(Style::default().fg(Color::Blue))
        .block(Block::default().borders(Borders::ALL).title("说明"));

    f.render_widget(note, area);
}

/// 绘制BaseAttr
fn draw_base_attr(f: &mut Frame, app: &EditorApp, area: Rect) {
    let mut lines = Vec::new();

    if let Some(record) = app.current_record() {
        if let Some(base_attr) = get_map(record, "BaseAttr") {
            let (str_val, str_style) = format_field_value(app, &["BaseAttr", "Str"], get_i64(base_attr, "Str"));
            let (dex_val, dex_style) = format_field_value(app, &["BaseAttr", "Dex"], get_i64(base_attr, "Dex"));
            let (mind_val, mind_style) = format_field_value(app, &["BaseAttr", "Mind"], get_i64(base_attr, "Mind"));
            let (con_val, con_style) = format_field_value(app, &["BaseAttr", "Con"], get_i64(base_attr, "Con"));
            let (hp_val, hp_style) = format_field_value(app, &["BaseAttr", "Hp"], get_i64(base_attr, "Hp"));
            let (mp_val, mp_style) = format_field_value(app, &["BaseAttr", "Mp"], get_i64(base_attr, "Mp"));

            lines.push(ratatui::text::Line::from(vec![
                ratatui::text::Span::styled("力量(Str): ", Style::default().fg(Color::White)),
                ratatui::text::Span::styled(str_val, str_style),
                ratatui::text::Span::raw("    "),
                ratatui::text::Span::styled("敏捷(Dex): ", Style::default().fg(Color::White)),
                ratatui::text::Span::styled(dex_val, dex_style),
                ratatui::text::Span::raw("    "),
                ratatui::text::Span::styled("智力(Mind): ", Style::default().fg(Color::White)),
                ratatui::text::Span::styled(mind_val, mind_style),
                ratatui::text::Span::raw("    "),
                ratatui::text::Span::styled("体质(Con): ", Style::default().fg(Color::White)),
                ratatui::text::Span::styled(con_val, con_style),
            ]));
            lines.push(ratatui::text::Line::from(vec![
                ratatui::text::Span::styled("基础HP: ", Style::default().fg(Color::White)),
                ratatui::text::Span::styled(hp_val, hp_style),
                ratatui::text::Span::raw("    "),
                ratatui::text::Span::styled("基础MP: ", Style::default().fg(Color::White)),
                ratatui::text::Span::styled(mp_val, mp_style),
            ]));
        }
    }

    if lines.is_empty() {
        lines.push(ratatui::text::Line::from(ratatui::text::Span::styled(
            "无数据",
            Style::default().fg(Color::DarkGray),
        )));
    }

    let title = "基础属性 (BaseAttr) ★核心★";
    let attr = Paragraph::new(lines).block(
        Block::default()
            .borders(Borders::ALL)
            .title(title)
            .title_style(Style::default().fg(Color::Green).add_modifier(Modifier::BOLD)),
    );

    f.render_widget(attr, area);
}

/// 绘制PermanentFightAttr
fn draw_permanent_attr(f: &mut Frame, app: &EditorApp, area: Rect) {
    let mut lines = Vec::new();

    if let Some(record) = app.current_record() {
        if let Some(pfa) = get_map(record, "PermanentFightAttr") {
            let (str_val, str_style) = format_field_value(app, &["PermanentFightAttr", "Str"], get_i64(pfa, "Str"));
            let (dex_val, dex_style) = format_field_value(app, &["PermanentFightAttr", "Dex"], get_i64(pfa, "Dex"));
            let (mind_val, mind_style) = format_field_value(app, &["PermanentFightAttr", "Mind"], get_i64(pfa, "Mind"));
            let (con_val, con_style) = format_field_value(app, &["PermanentFightAttr", "Con"], get_i64(pfa, "Con"));
            let (max_hp_val, max_hp_style) = format_field_value(app, &["PermanentFightAttr", "MaxHp"], get_i64(pfa, "MaxHp"));
            let (max_mp_val, max_mp_style) = format_field_value(app, &["PermanentFightAttr", "MaxMp"], get_i64(pfa, "MaxMp"));
            let (pa_val, pa_style) = format_field_value(app, &["PermanentFightAttr", "PhysicalAttack"], get_i64(pfa, "PhysicalAttack"));
            let (ma_val, ma_style) = format_field_value(app, &["PermanentFightAttr", "MagicAttack"], get_i64(pfa, "MagicAttack"));
            let (def_val, def_style) = format_field_value(app, &["PermanentFightAttr", "Defense"], get_i64(pfa, "Defense"));
            let (spd_val, spd_style) = format_field_value(app, &["PermanentFightAttr", "Speed"], get_i64(pfa, "Speed"));
            let (crit_val, crit_style) = format_field_value(app, &["PermanentFightAttr", "CriticalRatio"], get_i64(pfa, "CriticalRatio"));
            let (dodge_val, dodge_style) = format_field_value(app, &["PermanentFightAttr", "DodgeRatio"], get_i64(pfa, "DodgeRatio"));

            lines.push(ratatui::text::Line::from(vec![
                ratatui::text::Span::styled("力量: ", Style::default().fg(Color::White)),
                ratatui::text::Span::styled(str_val, str_style),
                ratatui::text::Span::raw("  "),
                ratatui::text::Span::styled("敏捷: ", Style::default().fg(Color::White)),
                ratatui::text::Span::styled(dex_val, dex_style),
                ratatui::text::Span::raw("  "),
                ratatui::text::Span::styled("智力: ", Style::default().fg(Color::White)),
                ratatui::text::Span::styled(mind_val, mind_style),
                ratatui::text::Span::raw("  "),
                ratatui::text::Span::styled("体质: ", Style::default().fg(Color::White)),
                ratatui::text::Span::styled(con_val, con_style),
            ]));
            lines.push(ratatui::text::Line::from(vec![
                ratatui::text::Span::styled("HP加成: ", Style::default().fg(Color::White)),
                ratatui::text::Span::styled(max_hp_val, max_hp_style),
                ratatui::text::Span::raw("  "),
                ratatui::text::Span::styled("MP加成: ", Style::default().fg(Color::White)),
                ratatui::text::Span::styled(max_mp_val, max_mp_style),
            ]));
            lines.push(ratatui::text::Line::from(vec![
                ratatui::text::Span::styled("物攻: ", Style::default().fg(Color::White)),
                ratatui::text::Span::styled(pa_val, pa_style),
                ratatui::text::Span::raw("  "),
                ratatui::text::Span::styled("魔攻: ", Style::default().fg(Color::White)),
                ratatui::text::Span::styled(ma_val, ma_style),
                ratatui::text::Span::raw("  "),
                ratatui::text::Span::styled("防御: ", Style::default().fg(Color::White)),
                ratatui::text::Span::styled(def_val, def_style),
                ratatui::text::Span::raw("  "),
                ratatui::text::Span::styled("速度: ", Style::default().fg(Color::White)),
                ratatui::text::Span::styled(spd_val, spd_style),
            ]));
            lines.push(ratatui::text::Line::from(vec![
                ratatui::text::Span::styled("暴击率: ", Style::default().fg(Color::White)),
                ratatui::text::Span::styled(crit_val, crit_style),
                ratatui::text::Span::raw("  "),
                ratatui::text::Span::styled("闪避率: ", Style::default().fg(Color::White)),
                ratatui::text::Span::styled(dodge_val, dodge_style),
            ]));
        }
    }

    if lines.is_empty() {
        lines.push(ratatui::text::Line::from(ratatui::text::Span::styled(
            "无数据",
            Style::default().fg(Color::DarkGray),
        )));
    }

    let title = "永久加成 (PermanentFightAttr) ★核心★";
    let attr = Paragraph::new(lines).block(
        Block::default()
            .borders(Borders::ALL)
            .title(title)
            .title_style(Style::default().fg(Color::Green).add_modifier(Modifier::BOLD)),
    );

    f.render_widget(attr, area);
}

/// 绘制FightAttr (只读)
fn draw_fight_attr(f: &mut Frame, app: &EditorApp, area: Rect) {
    let mut lines = Vec::new();

    if let Some(record) = app.current_record() {
        if let Some(fa) = get_map(record, "FightAttr") {
            lines.push(ratatui::text::Line::from(ratatui::text::Span::styled(
                "战斗属性 (FightAttr)  [自动计算 · 只读参考]",
                Style::default().fg(Color::DarkGray).add_modifier(Modifier::ITALIC),
            )));
            lines.push(ratatui::text::Line::from(""));
            lines.push(ratatui::text::Line::from(vec![
                ratatui::text::Span::styled("当前HP: ", Style::default().fg(Color::White)),
                ratatui::text::Span::styled(
                    get_i64(fa, "Hp").map_or("-".to_string(), |v| v.to_string()),
                    Style::default().fg(Color::DarkGray),
                ),
                ratatui::text::Span::raw("  "),
                ratatui::text::Span::styled("最大HP: ", Style::default().fg(Color::White)),
                ratatui::text::Span::styled(
                    get_i64(fa, "MaxHp").map_or("-".to_string(), |v| v.to_string()),
                    Style::default().fg(Color::DarkGray),
                ),
                ratatui::text::Span::raw("  "),
                ratatui::text::Span::styled("当前MP: ", Style::default().fg(Color::White)),
                ratatui::text::Span::styled(
                    get_i64(fa, "Mp").map_or("-".to_string(), |v| v.to_string()),
                    Style::default().fg(Color::DarkGray),
                ),
                ratatui::text::Span::raw("  "),
                ratatui::text::Span::styled("最大MP: ", Style::default().fg(Color::White)),
                ratatui::text::Span::styled(
                    get_i64(fa, "MaxMp").map_or("-".to_string(), |v| v.to_string()),
                    Style::default().fg(Color::DarkGray),
                ),
            ]));
            lines.push(ratatui::text::Line::from(vec![
                ratatui::text::Span::styled("力量: ", Style::default().fg(Color::White)),
                ratatui::text::Span::styled(
                    get_i64(fa, "Str").map_or("-".to_string(), |v| v.to_string()),
                    Style::default().fg(Color::DarkGray),
                ),
                ratatui::text::Span::raw("  "),
                ratatui::text::Span::styled("敏捷: ", Style::default().fg(Color::White)),
                ratatui::text::Span::styled(
                    get_i64(fa, "Dex").map_or("-".to_string(), |v| v.to_string()),
                    Style::default().fg(Color::DarkGray),
                ),
                ratatui::text::Span::raw("  "),
                ratatui::text::Span::styled("智力: ", Style::default().fg(Color::White)),
                ratatui::text::Span::styled(
                    get_i64(fa, "Mind").map_or("-".to_string(), |v| v.to_string()),
                    Style::default().fg(Color::DarkGray),
                ),
                ratatui::text::Span::raw("  "),
                ratatui::text::Span::styled("体质: ", Style::default().fg(Color::White)),
                ratatui::text::Span::styled(
                    get_i64(fa, "Con").map_or("-".to_string(), |v| v.to_string()),
                    Style::default().fg(Color::DarkGray),
                ),
            ]));
            lines.push(ratatui::text::Line::from(vec![
                ratatui::text::Span::styled("物攻: ", Style::default().fg(Color::White)),
                ratatui::text::Span::styled(
                    get_i64(fa, "PhysicalAttack").map_or("-".to_string(), |v| v.to_string()),
                    Style::default().fg(Color::DarkGray),
                ),
                ratatui::text::Span::raw("  "),
                ratatui::text::Span::styled("魔攻: ", Style::default().fg(Color::White)),
                ratatui::text::Span::styled(
                    get_i64(fa, "MagicAttack").map_or("-".to_string(), |v| v.to_string()),
                    Style::default().fg(Color::DarkGray),
                ),
                ratatui::text::Span::raw("  "),
                ratatui::text::Span::styled("防御: ", Style::default().fg(Color::White)),
                ratatui::text::Span::styled(
                    get_i64(fa, "Defense").map_or("-".to_string(), |v| v.to_string()),
                    Style::default().fg(Color::DarkGray),
                ),
                ratatui::text::Span::raw("  "),
                ratatui::text::Span::styled("速度: ", Style::default().fg(Color::White)),
                ratatui::text::Span::styled(
                    get_i64(fa, "Speed").map_or("-".to_string(), |v| v.to_string()),
                    Style::default().fg(Color::DarkGray),
                ),
            ]));
        }
    }

    if lines.is_empty() {
        lines.push(ratatui::text::Line::from(ratatui::text::Span::styled(
            "无数据",
            Style::default().fg(Color::DarkGray),
        )));
    }

    let title = "战斗属性 (FightAttr)  [只读参考]";
    let attr = Paragraph::new(lines).block(
        Block::default()
            .borders(Borders::ALL)
            .title(title)
            .title_style(Style::default().fg(Color::DarkGray)),
    );

    f.render_widget(attr, area);
}
