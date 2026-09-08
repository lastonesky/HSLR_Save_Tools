use ratatui::Frame;
use ratatui::layout::{Constraint, Direction, Layout, Rect};
use ratatui::style::{Color, Style};
use ratatui::widgets::{Block, Borders, Paragraph};

use crate::editor::EditorApp;
use crate::save::*;

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

/// 绘制战场属性页
pub fn draw(f: &mut Frame, app: &EditorApp, area: Rect) {
    let chunks = Layout::default()
        .direction(Direction::Vertical)
        .constraints([
            Constraint::Length(12), // 战场属性
            Constraint::Length(5),  // 快捷操作
            Constraint::Length(4),  // 警告
        ])
        .split(area);

    // 战场属性
    draw_battle_attrs(f, app, chunks[0]);

    // 快捷操作
    draw_quick_actions(f, chunks[1]);

    // 警告
    draw_warning(f, chunks[2]);
}

/// 绘制战场属性
fn draw_battle_attrs(f: &mut Frame, app: &EditorApp, area: Rect) {
    let mut lines = Vec::new();

    if let Some(entity) = app.current_entity() {
        let (hp_val, hp_style) = format_field_value(app, &["Hp"], get_i64(entity, "Hp"));
        let (max_hp_val, max_hp_style) = format_field_value(app, &["MaxHp"], get_i64(entity, "MaxHp"));
        let (mp_val, mp_style) = format_field_value(app, &["Mp"], get_i64(entity, "Mp"));
        let (max_mp_val, max_mp_style) = format_field_value(app, &["MaxMp"], get_i64(entity, "MaxMp"));
        let (level_val, level_style) = format_field_value(app, &["Level"], get_i64(entity, "Level"));
        let (exp_val, exp_style) = format_field_value(app, &["Exp"], get_i64(entity, "Exp"));

        lines.push(ratatui::text::Line::from(vec![
            ratatui::text::Span::styled("当前HP: ", Style::default().fg(Color::White)),
            ratatui::text::Span::styled(hp_val, hp_style),
            ratatui::text::Span::raw("  "),
            ratatui::text::Span::styled("最大HP: ", Style::default().fg(Color::White)),
            ratatui::text::Span::styled(max_hp_val, max_hp_style),
            ratatui::text::Span::raw("  "),
            ratatui::text::Span::styled("当前MP: ", Style::default().fg(Color::White)),
            ratatui::text::Span::styled(mp_val, mp_style),
            ratatui::text::Span::raw("  "),
            ratatui::text::Span::styled("最大MP: ", Style::default().fg(Color::White)),
            ratatui::text::Span::styled(max_mp_val, max_mp_style),
        ]));

        lines.push(ratatui::text::Line::from(vec![
            ratatui::text::Span::styled("等级: ", Style::default().fg(Color::White)),
            ratatui::text::Span::styled(level_val, level_style),
            ratatui::text::Span::raw("  "),
            ratatui::text::Span::styled("经验: ", Style::default().fg(Color::White)),
            ratatui::text::Span::styled(exp_val, exp_style),
        ]));

        if let Some(fight_attr) = get_map(entity, "FightAttr") {
            let (str_val, str_style) = format_field_value(app, &["FightAttr", "Str"], get_i64(fight_attr, "Str"));
            let (dex_val, dex_style) = format_field_value(app, &["FightAttr", "Dex"], get_i64(fight_attr, "Dex"));
            let (mind_val, mind_style) = format_field_value(app, &["FightAttr", "Mind"], get_i64(fight_attr, "Mind"));
            let (con_val, con_style) = format_field_value(app, &["FightAttr", "Con"], get_i64(fight_attr, "Con"));
            let (pa_val, pa_style) = format_field_value(app, &["FightAttr", "PhysicalAttack"], get_i64(fight_attr, "PhysicalAttack"));
            let (ma_val, ma_style) = format_field_value(app, &["FightAttr", "MagicAttack"], get_i64(fight_attr, "MagicAttack"));
            let (def_val, def_style) = format_field_value(app, &["FightAttr", "Defense"], get_i64(fight_attr, "Defense"));
            let (spd_val, spd_style) = format_field_value(app, &["FightAttr", "Speed"], get_i64(fight_attr, "Speed"));
            let (move_val, move_style) = format_field_value(app, &["FightAttr", "Move"], get_i64(fight_attr, "Move"));
            let (crit_val, crit_style) = format_field_value(app, &["FightAttr", "CriticalRatio"], get_i64(fight_attr, "CriticalRatio"));
            let (dodge_val, dodge_style) = format_field_value(app, &["FightAttr", "DodgeRatio"], get_i64(fight_attr, "DodgeRatio"));

            lines.push(ratatui::text::Line::from(""));
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
                ratatui::text::Span::styled("移动力: ", Style::default().fg(Color::White)),
                ratatui::text::Span::styled(move_val, move_style),
                ratatui::text::Span::raw("  "),
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

    let title = "战场属性 (charEntitiesMap)";
    let attr = Paragraph::new(lines).block(
        Block::default()
            .borders(Borders::ALL)
            .title(title),
    );

    f.render_widget(attr, area);
}

/// 绘制快捷操作
fn draw_quick_actions(f: &mut Frame, area: Rect) {
    let actions = vec![
        "H: 一键满血满蓝",
        "M: 战场属性MAX(本战)",
        "L: Lv99 + 满经验",
    ];

    let lines: Vec<ratatui::text::Line> = actions
        .iter()
        .map(|a| ratatui::text::Line::from(ratatui::text::Span::styled(*a, Style::default().fg(Color::White))))
        .collect();

    let help = Paragraph::new(lines).block(
        Block::default()
            .borders(Borders::ALL)
            .title("快捷操作"),
    );

    f.render_widget(help, area);
}

/// 绘制警告
fn draw_warning(f: &mut Frame, area: Rect) {
    let warning = Paragraph::new("⚠ 战场属性仅当前战斗有效，下次进图会重算。\n永久修改请到「存档属性」页或用底部「全队满属性(持久)」按钮。")
        .style(Style::default().fg(Color::Rgb(176, 96, 0)))
        .block(Block::default().borders(Borders::ALL).title("警告"));

    f.render_widget(warning, area);
}
