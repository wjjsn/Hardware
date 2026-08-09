#pragma once
namespace Hardware
{
template <typename motor, typename in1_gpio, typename in2_gpio>
struct TB6612
{
	static void init()
	{
		in1_gpio::init();
		in2_gpio::init();
		motor::init();
		stop();
	}
	static void forward(float speed)
	{
		in1_gpio::set();
		in2_gpio::clear();
		motor::set_speed(speed);
	}
	static void backward(float speed)
	{
		in1_gpio::clear();
		in2_gpio::set();
		motor::set_speed(speed);
	}
	static void stop()
	{
		in1_gpio::clear();
		in2_gpio::clear();
		motor::set_speed(0);
	}
	static void brake()
	{
		in1_gpio::set();
		in2_gpio::set();
		motor::set_speed(0);
	}
};
}
