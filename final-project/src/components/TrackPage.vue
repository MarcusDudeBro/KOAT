<template>
  <div class="track-view container d-flex flex-column justify-content-center align-items-center">
    <h1 class="text-center mb-5">Track KOAT</h1>
    <!-- <div class="compass-arrow" :style="arrowStyle"></div> -->
    <CompassPage />
    <br>
    <h2>{{distance}} feet away</h2>
    <!-- <button @click="setTargetDirection">Set Target Direction</button> -->
  </div>
</template>

<script>
  import CompassPage from './CompassPage.vue';
  export default {
    components: {
      CompassPage
    },
    name: 'TrackPage',
    data() {
      return {
        heading: null,
        longitude: null,
        latitude: null,
        imu: null,
        distance: null,
        arrowDirection: null,
        targetDirection: null
      };
    },
    async created() {
      await this.getGeolocationInformation();
      await this.getImuPosition();
      this.convertToCartesian();
      this.calculateDistance();
      window.addEventListener('deviceorientation', this.handleDeviceOrientation, true);

      // Add setInterval to call getGeolocationInformation() every 2 seconds
      setInterval(() => {
        this.getGeolocationInformation();
        this.convertToCartesian();
      }, 2000);
    },
    methods: {
      async getImuPosition() {
        try {
          const response = await fetch('http://54.215.244.83:5000/position');
          const data = await response.json();
          this.imu = data;
        } catch (error) {
          console.error(error);
        }
      },
      async getGeolocationInformation() {
        const API_KEY = 'AIzaSyBiiyv5ZdtzWygUpXX-1c5N66GZWtzj96U';
        const API_URL = `https://www.googleapis.com/geolocation/v1/geolocate?key=${API_KEY}`;
        try {
          const response = await fetch(API_URL, {
            method: 'POST',
          });
          const data = await response.json();
          const { lat, lng } = data.location;
          this.latitude = lat;
          this.longitude = lng;
        } catch (error) {
          console.error(error);
        }
      },
      convertToCartesian() {
        let x = 6371e3 * Math.cos(this.latitude) * Math.cos(this.longitude);
        let y = 6371e3 * Math.cos(this.latitude) * Math.sin(this.longitude);
        let z = 6371e3 * Math.sin(this.latitude);
        console.log(x, y, z);
        let posX = x + this.imu;
        let posY = y + this.imu;
        let posZ = z;
        this.calculateDistance(posX, posY, posZ, z, x, y);
      },
      calculateDistance(posX, posY, posZ, z, x, y) {
        const Δx = posX - x;
        const Δy = posY - y;
        const Δz = posZ - z;
        const distance = Math.sqrt(Math.pow(Δx, 2) + Math.pow(Δy, 2) + Math.pow(Δz, 2));
        this.distance = distance.toFixed(0);
        console.log(distance);
      }
    },
    computed: {
      arrowStyle() {
        return {
          transform: `rotate(${this.arrowDirection}deg)`,
          width: '80px',
          height: '80px',
          border: 'solid navy',
          borderWidth: '0 80px 120px 80px',
          borderColor: `transparent transparent navy transparent`
        };
      }
    }
  };
</script>

<style scoped>
.track-view {
  background-color: white;
  color: navy;
  height: 100vh;
  position: relative;
}

button {
  position: absolute;
  bottom: 0;
}
</style>
