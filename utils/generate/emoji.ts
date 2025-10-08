export class Emoji {
  constructor(
    public codepoint: number,
    public emoji: boolean,
    public emoji_presentation: boolean,
    public emoji_modifier: boolean,
    public emoji_modifier_base: boolean,
    public emoji_component: boolean,
    public extended_pictographic: boolean
  ) {}
}
