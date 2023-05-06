<template>
  <div>
    <div class="compass-container" ref="compassContainer">
      <div class="compass-arrow" ref="arrow"></div>
    </div>
    <button @click="setTargetDirection">Set Target Direction</button>
  </div>
</template>

<script>
export default {
  data() {
    return {
      targetDirection: null,
      arrowDirection: null,
      heading: null,
      longitude: null,
      latitude: null,
    };
  },
  mounted() {
    if (window.DeviceOrientationEvent) {
      window.addEventListener('deviceorientationabsolute', this.handleDeviceOrientation);
      console.log('Device orientation event listener added');
    } else {
      console.log('Device orientation not supported');
    }
  },
  beforeUnmount() {
    window.removeEventListener('deviceorientationabsolute', this.handleDeviceOrientation);
  },
  methods: {
    handleDeviceOrientation(event) {
      const { alpha } = event;
      this.heading = alpha;
      if (this.targetDirection !== null) {
        this.arrowDirection = this.targetDirection - this.heading;
      } else {
        this.arrowDirection = -alpha; // Rotate arrow in x-y plane only
      }
      this.arrowDirection = (this.arrowDirection + 360) % 360;

      const arrow = this.$refs.arrow;
      arrow.style.transform = `rotate(${this.arrowDirection}deg)`;
    },
    setTargetDirection() {
      if (window.DeviceOrientationEvent && event.absolute) {
        const { alpha } = event;
        this.targetDirection = alpha;
        this.arrowDirection = -alpha;
        const arrow = this.$refs.arrow;
        arrow.style.transform = `rotate(${this.arrowDirection}deg)`;
      }
    },
  },
};
</script>

<style>
.compass-container {
  width: 200px;
  height: 200px;
  position: relative;
  margin: auto;
  margin-top: 50px;
}

.compass-arrow {
  width: 0;
  height: 0;
  border-style: solid;
  border-width: 0 80px 120px 80px;
  border-color: transparent transparent navy transparent;
  position: absolute;
  top: 50%;
  left: 50%;
  transform-origin: top center;
}
</style>
