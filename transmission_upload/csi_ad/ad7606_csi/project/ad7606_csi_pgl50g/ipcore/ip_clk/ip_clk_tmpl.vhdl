-- Created by IP Generator (Version 2021.1-SP7.1 build 90533)
-- Instantiation Template
--
-- Insert the following codes into your VHDL file.
--   * Change the_instance_name to your own instance name.
--   * Change the net names in the port map.


COMPONENT ip_clk
  PORT (
    clkin1 : IN STD_LOGIC;
    pll_lock : OUT STD_LOGIC;
    clkout0 : OUT STD_LOGIC;
    clkout1 : OUT STD_LOGIC;
    clkout2 : OUT STD_LOGIC
  );
END COMPONENT;


the_instance_name : ip_clk
  PORT MAP (
    clkin1 => clkin1,
    pll_lock => pll_lock,
    clkout0 => clkout0,
    clkout1 => clkout1,
    clkout2 => clkout2
  );
